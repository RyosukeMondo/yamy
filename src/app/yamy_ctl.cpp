//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// yamy_ctl.cpp - Command-line tool for controlling running YAMY instance
//
// Usage:
//   yamy-ctl reload [--config NAME]  - Reload configuration
//   yamy-ctl stop                    - Stop the engine
//   yamy-ctl start                   - Start the engine
//   yamy-ctl status                  - Get engine status
//   yamy-ctl --help                  - Show help
//

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <cerrno>
#include <getopt.h>

namespace {

/// Default socket path for engine control
constexpr const char* DEFAULT_SOCKET_PATH = "/tmp/yamy-engine.sock";

/// Default timeout for waiting for response (milliseconds)
constexpr int DEFAULT_TIMEOUT_MS = 5000;

/// Exit codes
enum ExitCode {
    SUCCESS = 0,
    ENGINE_NOT_RUNNING = 1,
    COMMAND_FAILED = 2,
    INVALID_ARGS = 3
};

/// IPC Message Types (must match ipc_messages.h)
enum MessageType : uint32_t {
    CmdReload = 0x2001,
    CmdStop = 0x2002,
    CmdStart = 0x2003,
    CmdGetStatus = 0x2004,
    RspOk = 0x2100,
    RspError = 0x2101,
    RspStatus = 0x2102
};

/// Wire protocol message header
struct MessageHeader {
    uint32_t type;
    uint32_t dataSize;
};

/// Print usage information
void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <command> [options]\n"
              << "\n"
              << "Commands:\n"
              << "  reload [--config NAME]  Reload configuration (optionally switch to NAME)\n"
              << "  stop                    Stop the key remapping engine\n"
              << "  start                   Start the key remapping engine\n"
              << "  status                  Show engine status\n"
              << "\n"
              << "Options:\n"
              << "  -c, --config NAME       Specify configuration name for reload\n"
              << "  -s, --socket PATH       Use custom socket path (default: " << DEFAULT_SOCKET_PATH << ")\n"
              << "  -t, --timeout MS        Response timeout in milliseconds (default: " << DEFAULT_TIMEOUT_MS << ")\n"
              << "  -h, --help              Show this help message\n"
              << "\n"
              << "Exit codes:\n"
              << "  0  Success\n"
              << "  1  YAMY engine is not running\n"
              << "  2  Command failed\n"
              << "  3  Invalid arguments\n"
              << "\n"
              << "Examples:\n"
              << "  " << progName << " status\n"
              << "  " << progName << " reload\n"
              << "  " << progName << " reload --config work\n"
              << "  " << progName << " stop\n";
}

/// Connect to YAMY engine socket
/// @return Socket file descriptor, or -1 on failure
int connectToEngine(const char* socketPath) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Error: Failed to create socket: " << std::strerror(errno) << "\n";
        return -1;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);

    if (connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
        if (errno == ECONNREFUSED || errno == ENOENT) {
            std::cerr << "Error: YAMY engine is not running\n";
        } else {
            std::cerr << "Error: Failed to connect to engine: " << std::strerror(errno) << "\n";
        }
        close(sock);
        return -1;
    }

    return sock;
}

/// Send message to engine
/// @return true on success
bool sendMessage(int sock, MessageType type, const std::string& data = "") {
    MessageHeader header;
    header.type = static_cast<uint32_t>(type);
    header.dataSize = static_cast<uint32_t>(data.size());

    // Send header
    ssize_t sent = send(sock, &header, sizeof(header), MSG_NOSIGNAL);
    if (sent != sizeof(header)) {
        std::cerr << "Error: Failed to send message header: " << std::strerror(errno) << "\n";
        return false;
    }

    // Send data if present
    if (!data.empty()) {
        sent = send(sock, data.c_str(), data.size(), MSG_NOSIGNAL);
        if (sent != static_cast<ssize_t>(data.size())) {
            std::cerr << "Error: Failed to send message data: " << std::strerror(errno) << "\n";
            return false;
        }
    }

    return true;
}

/// Receive response from engine
/// @return true on success, response stored in type and data
bool receiveResponse(int sock, int timeoutMs, MessageType& type, std::string& data) {
    // Wait for data with timeout
    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLIN;

    int pollResult = poll(&pfd, 1, timeoutMs);
    if (pollResult == 0) {
        std::cerr << "Error: Timeout waiting for response from engine\n";
        return false;
    }
    if (pollResult < 0) {
        std::cerr << "Error: poll() failed: " << std::strerror(errno) << "\n";
        return false;
    }

    // Read header
    MessageHeader header;
    ssize_t received = recv(sock, &header, sizeof(header), MSG_WAITALL);
    if (received != sizeof(header)) {
        if (received == 0) {
            std::cerr << "Error: Connection closed by engine\n";
        } else {
            std::cerr << "Error: Failed to receive response header: " << std::strerror(errno) << "\n";
        }
        return false;
    }

    type = static_cast<MessageType>(header.type);

    // Read data if present
    data.clear();
    if (header.dataSize > 0) {
        // Sanity check - don't allow huge messages
        if (header.dataSize > 1024 * 1024) {
            std::cerr << "Error: Response data too large (" << header.dataSize << " bytes)\n";
            return false;
        }

        data.resize(header.dataSize);
        received = recv(sock, &data[0], header.dataSize, MSG_WAITALL);
        if (received != static_cast<ssize_t>(header.dataSize)) {
            std::cerr << "Error: Failed to receive response data: " << std::strerror(errno) << "\n";
            return false;
        }
    }

    return true;
}

/// Execute reload command
int cmdReload(int sock, int timeoutMs, const std::string& configName) {
    if (!sendMessage(sock, MessageType::CmdReload, configName)) {
        return COMMAND_FAILED;
    }

    MessageType respType;
    std::string respData;
    if (!receiveResponse(sock, timeoutMs, respType, respData)) {
        return COMMAND_FAILED;
    }

    if (respType == MessageType::RspOk) {
        if (configName.empty()) {
            std::cout << "Configuration reloaded successfully\n";
        } else {
            std::cout << "Switched to configuration: " << configName << "\n";
        }
        if (!respData.empty()) {
            std::cout << respData << "\n";
        }
        return SUCCESS;
    } else if (respType == MessageType::RspError) {
        std::cerr << "Error: " << (respData.empty() ? "Failed to reload configuration" : respData) << "\n";
        return COMMAND_FAILED;
    }

    std::cerr << "Error: Unexpected response from engine\n";
    return COMMAND_FAILED;
}

/// Execute stop command
int cmdStop(int sock, int timeoutMs) {
    if (!sendMessage(sock, MessageType::CmdStop)) {
        return COMMAND_FAILED;
    }

    MessageType respType;
    std::string respData;
    if (!receiveResponse(sock, timeoutMs, respType, respData)) {
        return COMMAND_FAILED;
    }

    if (respType == MessageType::RspOk) {
        std::cout << "Engine stopped\n";
        return SUCCESS;
    } else if (respType == MessageType::RspError) {
        std::cerr << "Error: " << (respData.empty() ? "Failed to stop engine" : respData) << "\n";
        return COMMAND_FAILED;
    }

    std::cerr << "Error: Unexpected response from engine\n";
    return COMMAND_FAILED;
}

/// Execute start command
int cmdStart(int sock, int timeoutMs) {
    if (!sendMessage(sock, MessageType::CmdStart)) {
        return COMMAND_FAILED;
    }

    MessageType respType;
    std::string respData;
    if (!receiveResponse(sock, timeoutMs, respType, respData)) {
        return COMMAND_FAILED;
    }

    if (respType == MessageType::RspOk) {
        std::cout << "Engine started\n";
        return SUCCESS;
    } else if (respType == MessageType::RspError) {
        std::cerr << "Error: " << (respData.empty() ? "Failed to start engine" : respData) << "\n";
        return COMMAND_FAILED;
    }

    std::cerr << "Error: Unexpected response from engine\n";
    return COMMAND_FAILED;
}

/// Execute status command
int cmdStatus(int sock, int timeoutMs) {
    if (!sendMessage(sock, MessageType::CmdGetStatus)) {
        return COMMAND_FAILED;
    }

    MessageType respType;
    std::string respData;
    if (!receiveResponse(sock, timeoutMs, respType, respData)) {
        return COMMAND_FAILED;
    }

    if (respType == MessageType::RspStatus || respType == MessageType::RspOk) {
        if (!respData.empty()) {
            std::cout << respData << "\n";
        } else {
            std::cout << "Engine is running\n";
        }
        return SUCCESS;
    } else if (respType == MessageType::RspError) {
        std::cerr << "Error: " << (respData.empty() ? "Failed to get status" : respData) << "\n";
        return COMMAND_FAILED;
    }

    std::cerr << "Error: Unexpected response from engine\n";
    return COMMAND_FAILED;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    const char* socketPath = DEFAULT_SOCKET_PATH;
    int timeoutMs = DEFAULT_TIMEOUT_MS;
    std::string configName;

    // Long options
    static struct option longOpts[] = {
        {"config",  required_argument, nullptr, 'c'},
        {"socket",  required_argument, nullptr, 's'},
        {"timeout", required_argument, nullptr, 't'},
        {"help",    no_argument,       nullptr, 'h'},
        {nullptr,   0,                 nullptr, 0}
    };

    // Parse options
    int opt;
    while ((opt = getopt_long(argc, argv, "c:s:t:h", longOpts, nullptr)) != -1) {
        switch (opt) {
            case 'c':
                configName = optarg;
                break;
            case 's':
                socketPath = optarg;
                break;
            case 't':
                timeoutMs = std::atoi(optarg);
                if (timeoutMs <= 0) {
                    std::cerr << "Error: Invalid timeout value\n";
                    return INVALID_ARGS;
                }
                break;
            case 'h':
                printUsage(argv[0]);
                return SUCCESS;
            default:
                printUsage(argv[0]);
                return INVALID_ARGS;
        }
    }

    // Check for command
    if (optind >= argc) {
        std::cerr << "Error: No command specified\n\n";
        printUsage(argv[0]);
        return INVALID_ARGS;
    }

    std::string command = argv[optind];

    // Validate command
    if (command != "reload" && command != "stop" && command != "start" && command != "status") {
        std::cerr << "Error: Unknown command: " << command << "\n\n";
        printUsage(argv[0]);
        return INVALID_ARGS;
    }

    // Connect to engine
    int sock = connectToEngine(socketPath);
    if (sock < 0) {
        return ENGINE_NOT_RUNNING;
    }

    // Execute command
    int result;
    if (command == "reload") {
        result = cmdReload(sock, timeoutMs, configName);
    } else if (command == "stop") {
        result = cmdStop(sock, timeoutMs);
    } else if (command == "start") {
        result = cmdStart(sock, timeoutMs);
    } else if (command == "status") {
        result = cmdStatus(sock, timeoutMs);
    } else {
        result = INVALID_ARGS;
    }

    close(sock);
    return result;
}
