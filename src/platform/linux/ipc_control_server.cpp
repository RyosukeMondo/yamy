//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ipc_control_server.cpp - IPC server for yamy-ctl control commands
//

#include "ipc_control_server.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cerrno>
#include <iostream>

namespace yamy::platform {

namespace {

/// Wire protocol message types (must match yamy_ctl.cpp and ipc_messages.h)
enum MessageType : uint32_t {
    CmdReload = 0x2001,
    CmdStop = 0x2002,
    CmdStart = 0x2003,
    CmdGetStatus = 0x2004,
    CmdGetConfig = 0x2005,
    CmdGetKeymaps = 0x2006,
    CmdGetMetrics = 0x2007,
    RspOk = 0x2100,
    RspError = 0x2101,
    RspStatus = 0x2102,
    RspConfig = 0x2103,
    RspKeymaps = 0x2104,
    RspMetrics = 0x2105
};

/// Wire protocol message header
struct MessageHeader {
    uint32_t type;
    uint32_t dataSize;
};

/// Receive all bytes from socket
bool recvAll(int fd, void* buf, size_t len) {
    char* ptr = static_cast<char*>(buf);
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t n = recv(fd, ptr, remaining, MSG_WAITALL);
        if (n <= 0) {
            return false;
        }
        ptr += n;
        remaining -= n;
    }
    return true;
}

/// Send all bytes to socket
bool sendAll(int fd, const void* buf, size_t len) {
    const char* ptr = static_cast<const char*>(buf);
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t n = send(fd, ptr, remaining, MSG_NOSIGNAL);
        if (n <= 0) {
            return false;
        }
        ptr += n;
        remaining -= n;
    }
    return true;
}

/// Send response to client
bool sendResponse(int fd, uint32_t type, const std::string& data) {
    MessageHeader header;
    header.type = type;
    header.dataSize = static_cast<uint32_t>(data.size());

    if (!sendAll(fd, &header, sizeof(header))) {
        return false;
    }

    if (!data.empty()) {
        if (!sendAll(fd, data.c_str(), data.size())) {
            return false;
        }
    }

    return true;
}

} // anonymous namespace

IPCControlServer::IPCControlServer(const std::string& socketPath)
    : m_socketPath(socketPath)
    , m_serverFd(-1)
    , m_running(false)
{
}

IPCControlServer::~IPCControlServer() {
    stop();
}

void IPCControlServer::setCommandCallback(ControlCommandCallback callback) {
    m_callback = std::move(callback);
}

bool IPCControlServer::start() {
    if (m_running) {
        return true; // Already running
    }

    // Remove existing socket file
    unlink(m_socketPath.c_str());

    // Create socket
    m_serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_serverFd == -1) {
        std::cerr << "IPCControlServer: Failed to create socket: " << std::strerror(errno) << "\n";
        return false;
    }

    // Bind to socket path
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(m_serverFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
        std::cerr << "IPCControlServer: Failed to bind socket: " << std::strerror(errno) << "\n";
        close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    // Listen for connections
    if (listen(m_serverFd, 5) == -1) {
        std::cerr << "IPCControlServer: Failed to listen: " << std::strerror(errno) << "\n";
        close(m_serverFd);
        m_serverFd = -1;
        unlink(m_socketPath.c_str());
        return false;
    }

    // Start server thread
    m_running = true;
    m_serverThread = std::make_unique<std::thread>(&IPCControlServer::serverLoop, this);

    std::cout << "IPCControlServer: Listening on " << m_socketPath << "\n";
    return true;
}

void IPCControlServer::stop() {
    if (!m_running) {
        return;
    }

    m_running = false;

    // Close server socket to unblock accept()
    if (m_serverFd >= 0) {
        shutdown(m_serverFd, SHUT_RDWR);
        close(m_serverFd);
        m_serverFd = -1;
    }

    // Wait for server thread to finish
    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
    }
    m_serverThread.reset();

    // Remove socket file
    unlink(m_socketPath.c_str());

    std::cout << "IPCControlServer: Stopped\n";
}

bool IPCControlServer::isRunning() const {
    return m_running;
}

void IPCControlServer::serverLoop() {
    while (m_running) {
        // Use poll to allow timeout for shutdown check
        struct pollfd pfd;
        pfd.fd = m_serverFd;
        pfd.events = POLLIN;

        int pollResult = poll(&pfd, 1, 500); // 500ms timeout
        if (pollResult < 0) {
            if (errno == EINTR) {
                continue; // Interrupted, check m_running
            }
            if (m_running) {
                std::cerr << "IPCControlServer: poll() error: " << std::strerror(errno) << "\n";
            }
            break;
        }

        if (pollResult == 0) {
            continue; // Timeout, check m_running and retry
        }

        // Accept new connection
        int clientFd = accept(m_serverFd, nullptr, nullptr);
        if (clientFd < 0) {
            if (errno == EINTR || errno == ECONNABORTED) {
                continue;
            }
            if (m_running) {
                std::cerr << "IPCControlServer: accept() error: " << std::strerror(errno) << "\n";
            }
            break;
        }

        // Handle client (synchronously - simple protocol)
        handleClient(clientFd);
        close(clientFd);
    }
}

void IPCControlServer::handleClient(int clientFd) {
    // Read message header
    MessageHeader header;
    if (!recvAll(clientFd, &header, sizeof(header))) {
        std::cerr << "IPCControlServer: Failed to receive message header\n";
        return;
    }

    // Read message data
    std::string data;
    if (header.dataSize > 0) {
        // Sanity check
        if (header.dataSize > 1024 * 1024) {
            std::cerr << "IPCControlServer: Message data too large\n";
            sendResponse(clientFd, MessageType::RspError, "Message data too large");
            return;
        }

        data.resize(header.dataSize);
        if (!recvAll(clientFd, &data[0], header.dataSize)) {
            std::cerr << "IPCControlServer: Failed to receive message data\n";
            return;
        }
    }

    // Map message type to command
    ControlCommand cmd;
    switch (header.type) {
        case MessageType::CmdReload:
            cmd = ControlCommand::Reload;
            break;
        case MessageType::CmdStop:
            cmd = ControlCommand::Stop;
            break;
        case MessageType::CmdStart:
            cmd = ControlCommand::Start;
            break;
        case MessageType::CmdGetStatus:
            cmd = ControlCommand::GetStatus;
            break;
        case MessageType::CmdGetConfig:
            cmd = ControlCommand::GetConfig;
            break;
        case MessageType::CmdGetKeymaps:
            cmd = ControlCommand::GetKeymaps;
            break;
        case MessageType::CmdGetMetrics:
            cmd = ControlCommand::GetMetrics;
            break;
        default:
            sendResponse(clientFd, MessageType::RspError, "Unknown command");
            return;
    }

    // Execute command via callback
    ControlResult result;
    if (m_callback) {
        result = m_callback(cmd, data);
    } else {
        result.success = false;
        result.message = "No command handler registered";
    }

    // Send response with appropriate type
    if (!result.success) {
        sendResponse(clientFd, MessageType::RspError, result.message);
    } else if (cmd == ControlCommand::GetStatus) {
        sendResponse(clientFd, MessageType::RspStatus, result.message);
    } else if (cmd == ControlCommand::GetConfig) {
        sendResponse(clientFd, MessageType::RspConfig, result.message);
    } else if (cmd == ControlCommand::GetKeymaps) {
        sendResponse(clientFd, MessageType::RspKeymaps, result.message);
    } else if (cmd == ControlCommand::GetMetrics) {
        sendResponse(clientFd, MessageType::RspMetrics, result.message);
    } else {
        sendResponse(clientFd, MessageType::RspOk, result.message);
    }
}

} // namespace yamy::platform
