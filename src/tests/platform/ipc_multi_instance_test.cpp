//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ipc_multi_instance_test.cpp - Integration tests for multi-instance IPC (1.2.9)
//
// Tests the multi-instance IPC scenarios on Linux:
// - Single instance detection via Unix domain sockets
// - Reload command forwarding between instances
// - Exit command forwarding between instances
// - Socket file cleanup on exit
// - Error handling for connection failures
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <errno.h>
#include <vector>
#include <functional>
#include "../../platform/linux/ipc_linux.h"
#include "../../core/platform/ipc.h"
#include "../../core/platform/types.h"

namespace yamy::platform {

//=============================================================================
// IPC Command IDs (mirrors what the application would use)
//=============================================================================

namespace IPCCommand {
    constexpr uint32_t RELOAD = 1;      // Reload configuration
    constexpr uint32_t EXIT = 2;        // Exit application
    constexpr uint32_t STATUS = 3;      // Query status
    constexpr uint32_t PING = 4;        // Ping for single-instance check
}

//=============================================================================
// Mock IPC Server - Simulates a YAMY instance listening for commands
//=============================================================================

class MockIPCServer {
public:
    using CommandHandler = std::function<void(uint32_t cmd, const std::vector<uint8_t>& data)>;

    explicit MockIPCServer(const std::string& instanceId)
        : m_instanceId(instanceId)
        , m_socket(-1)
        , m_running(false)
    {
        char pathBuf[128];
        std::snprintf(pathBuf, sizeof(pathBuf), "/tmp/yamy_%s.sock", instanceId.c_str());
        m_socketPath = pathBuf;
    }

    ~MockIPCServer() {
        stop();
    }

    bool start() {
        // Clean up any existing socket file
        unlink(m_socketPath.c_str());

        m_socket = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_socket == -1) {
            return false;
        }

        struct sockaddr_un addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);

        if (bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            close(m_socket);
            m_socket = -1;
            return false;
        }

        if (listen(m_socket, 5) == -1) {
            close(m_socket);
            m_socket = -1;
            return false;
        }

        m_running = true;
        m_serverThread = std::thread(&MockIPCServer::serverLoop, this);
        return true;
    }

    void stop() {
        m_running = false;
        if (m_socket != -1) {
            shutdown(m_socket, SHUT_RDWR);
            close(m_socket);
            m_socket = -1;
        }
        if (m_serverThread.joinable()) {
            m_serverThread.join();
        }
        // Clean up socket file
        unlink(m_socketPath.c_str());
    }

    void setCommandHandler(CommandHandler handler) {
        m_handler = handler;
    }

    bool isRunning() const { return m_running; }
    const std::string& socketPath() const { return m_socketPath; }
    const std::string& instanceId() const { return m_instanceId; }

    // Get received commands for verification
    const std::vector<uint32_t>& receivedCommands() const { return m_receivedCommands; }
    void clearReceivedCommands() { m_receivedCommands.clear(); }

private:
    void serverLoop() {
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        while (m_running) {
            int clientSocket = accept(m_socket, nullptr, nullptr);
            if (clientSocket == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;  // Timeout, check if still running
                }
                break;  // Real error
            }

            handleClient(clientSocket);
            close(clientSocket);
        }
    }

    void handleClient(int clientSocket) {
        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        uint32_t id = 0;
        uint32_t size = 0;

        if (recv(clientSocket, &id, sizeof(id), MSG_WAITALL) != sizeof(id)) {
            return;
        }

        if (recv(clientSocket, &size, sizeof(size), MSG_WAITALL) != sizeof(size)) {
            return;
        }

        std::vector<uint8_t> data;
        if (size > 0) {
            data.resize(size);
            if (recv(clientSocket, data.data(), size, MSG_WAITALL) != static_cast<ssize_t>(size)) {
                return;
            }
        }

        m_receivedCommands.push_back(id);

        if (m_handler) {
            m_handler(id, data);
        }
    }

    std::string m_instanceId;
    std::string m_socketPath;
    int m_socket;
    std::atomic<bool> m_running;
    std::thread m_serverThread;
    CommandHandler m_handler;
    std::vector<uint32_t> m_receivedCommands;
};

//=============================================================================
// Test Fixture
//=============================================================================

class IPCMultiInstanceTest : public ::testing::Test {
protected:
    std::string testInstanceId;
    std::string socketPath;

    void SetUp() override {
        // Generate unique instance ID for this test
        testInstanceId = "test_" + std::to_string(getpid()) + "_" +
                        std::to_string(reinterpret_cast<uintptr_t>(this));

        char pathBuf[128];
        std::snprintf(pathBuf, sizeof(pathBuf), "/tmp/yamy_%s.sock", testInstanceId.c_str());
        socketPath = pathBuf;

        // Clean up any leftover socket
        unlink(socketPath.c_str());
    }

    void TearDown() override {
        unlink(socketPath.c_str());
    }

    // Helper to send a command to a running server
    bool sendCommand(const std::string& instanceId, uint32_t commandId,
                     const void* payload = nullptr, uint32_t payloadSize = 0) {
        char pathBuf[128];
        std::snprintf(pathBuf, sizeof(pathBuf), "/tmp/yamy_%s.sock", instanceId.c_str());

        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock == -1) return false;

        struct sockaddr_un addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, pathBuf, sizeof(addr.sun_path) - 1);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            close(sock);
            return false;
        }

        bool success = true;
        if (send(sock, &commandId, sizeof(commandId), MSG_NOSIGNAL) != sizeof(commandId)) {
            success = false;
        }
        if (success && send(sock, &payloadSize, sizeof(payloadSize), MSG_NOSIGNAL) != sizeof(payloadSize)) {
            success = false;
        }
        if (success && payloadSize > 0 && payload) {
            if (send(sock, payload, payloadSize, MSG_NOSIGNAL) != static_cast<ssize_t>(payloadSize)) {
                success = false;
            }
        }

        close(sock);
        return success;
    }

    // Check if a socket file exists (indicates instance is running)
    bool socketExists(const std::string& instanceId) {
        char pathBuf[128];
        std::snprintf(pathBuf, sizeof(pathBuf), "/tmp/yamy_%s.sock", instanceId.c_str());
        return access(pathBuf, F_OK) == 0;
    }

    // Attempt to connect to check if instance is actually listening
    bool canConnectTo(const std::string& instanceId) {
        char pathBuf[128];
        std::snprintf(pathBuf, sizeof(pathBuf), "/tmp/yamy_%s.sock", instanceId.c_str());

        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock == -1) return false;

        struct sockaddr_un addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, pathBuf, sizeof(addr.sun_path) - 1);

        bool connected = (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0);
        close(sock);
        return connected;
    }
};

//=============================================================================
// Single Instance Detection Tests
//=============================================================================

TEST_F(IPCMultiInstanceTest, DetectRunningInstance) {
    // Start a "first instance" server
    MockIPCServer firstInstance(testInstanceId);
    ASSERT_TRUE(firstInstance.start());

    // Wait for server to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Verify socket file exists (how a second instance would detect first)
    EXPECT_TRUE(socketExists(testInstanceId));

    // Verify we can connect (instance is actually listening)
    EXPECT_TRUE(canConnectTo(testInstanceId));
}

TEST_F(IPCMultiInstanceTest, NoInstanceRunning) {
    // No server started - simulate checking for existing instance
    EXPECT_FALSE(socketExists(testInstanceId));
    EXPECT_FALSE(canConnectTo(testInstanceId));
}

TEST_F(IPCMultiInstanceTest, StaleSocketFile) {
    // Create a stale socket file (no server listening)
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_NE(sock, -1);

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

    ASSERT_EQ(bind(sock, (struct sockaddr*)&addr, sizeof(addr)), 0);
    close(sock);  // Close without listening

    // Socket file exists but no one is listening
    EXPECT_TRUE(socketExists(testInstanceId));
    EXPECT_FALSE(canConnectTo(testInstanceId));
}

//=============================================================================
// Reload Command Tests
//=============================================================================

TEST_F(IPCMultiInstanceTest, SendReloadCommand) {
    MockIPCServer server(testInstanceId);
    std::atomic<bool> reloadReceived{false};

    server.setCommandHandler([&reloadReceived](uint32_t cmd, const std::vector<uint8_t>&) {
        if (cmd == IPCCommand::RELOAD) {
            reloadReceived = true;
        }
    });

    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Send reload command (simulating: yamy --reload)
    EXPECT_TRUE(sendCommand(testInstanceId, IPCCommand::RELOAD));

    // Wait for command to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(reloadReceived);
    EXPECT_EQ(server.receivedCommands().size(), 1u);
    EXPECT_EQ(server.receivedCommands()[0], IPCCommand::RELOAD);
}

TEST_F(IPCMultiInstanceTest, ReloadToNonExistentInstance) {
    // Try to send reload to an instance that doesn't exist
    EXPECT_FALSE(sendCommand(testInstanceId, IPCCommand::RELOAD));
}

//=============================================================================
// Exit Command Tests
//=============================================================================

TEST_F(IPCMultiInstanceTest, SendExitCommand) {
    MockIPCServer server(testInstanceId);
    std::atomic<bool> exitReceived{false};

    server.setCommandHandler([&exitReceived](uint32_t cmd, const std::vector<uint8_t>&) {
        if (cmd == IPCCommand::EXIT) {
            exitReceived = true;
        }
    });

    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Send exit command (simulating: yamy --exit)
    EXPECT_TRUE(sendCommand(testInstanceId, IPCCommand::EXIT));

    // Wait for command to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(exitReceived);
    EXPECT_EQ(server.receivedCommands().size(), 1u);
    EXPECT_EQ(server.receivedCommands()[0], IPCCommand::EXIT);
}

TEST_F(IPCMultiInstanceTest, ExitToNonExistentInstance) {
    // Try to send exit to an instance that doesn't exist
    EXPECT_FALSE(sendCommand(testInstanceId, IPCCommand::EXIT));
}

//=============================================================================
// Socket Cleanup Tests
//=============================================================================

TEST_F(IPCMultiInstanceTest, SocketCleanupOnStop) {
    MockIPCServer server(testInstanceId);
    ASSERT_TRUE(server.start());

    // Verify socket exists while running
    EXPECT_TRUE(socketExists(testInstanceId));

    // Stop the server
    server.stop();

    // Verify socket is cleaned up
    EXPECT_FALSE(socketExists(testInstanceId));
}

TEST_F(IPCMultiInstanceTest, SocketCleanupOnDestruction) {
    {
        MockIPCServer server(testInstanceId);
        ASSERT_TRUE(server.start());
        EXPECT_TRUE(socketExists(testInstanceId));
        // Server goes out of scope here
    }

    // Socket should be cleaned up after destructor
    EXPECT_FALSE(socketExists(testInstanceId));
}

TEST_F(IPCMultiInstanceTest, MultipleStartStopCycles) {
    for (int i = 0; i < 3; ++i) {
        MockIPCServer server(testInstanceId);
        ASSERT_TRUE(server.start()) << "Failed on cycle " << i;
        EXPECT_TRUE(socketExists(testInstanceId));
        server.stop();
        EXPECT_FALSE(socketExists(testInstanceId));
    }
}

//=============================================================================
// Command with Payload Tests
//=============================================================================

TEST_F(IPCMultiInstanceTest, SendCommandWithPayload) {
    MockIPCServer server(testInstanceId);
    std::vector<uint8_t> receivedPayload;

    server.setCommandHandler([&receivedPayload](uint32_t cmd, const std::vector<uint8_t>& data) {
        if (cmd == IPCCommand::RELOAD) {
            receivedPayload = data;
        }
    });

    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Send reload with config path payload
    const char* configPath = "/home/user/.yamy/custom.mayu";
    EXPECT_TRUE(sendCommand(testInstanceId, IPCCommand::RELOAD,
                           configPath, static_cast<uint32_t>(strlen(configPath) + 1)));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_EQ(receivedPayload.size(), strlen(configPath) + 1);
    EXPECT_STREQ(reinterpret_cast<const char*>(receivedPayload.data()), configPath);
}

//=============================================================================
// Multiple Commands in Sequence Tests
//=============================================================================

TEST_F(IPCMultiInstanceTest, MultipleCommandsSequential) {
    MockIPCServer server(testInstanceId);
    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Send multiple commands
    EXPECT_TRUE(sendCommand(testInstanceId, IPCCommand::PING));
    EXPECT_TRUE(sendCommand(testInstanceId, IPCCommand::RELOAD));
    EXPECT_TRUE(sendCommand(testInstanceId, IPCCommand::STATUS));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ASSERT_EQ(server.receivedCommands().size(), 3u);
    EXPECT_EQ(server.receivedCommands()[0], IPCCommand::PING);
    EXPECT_EQ(server.receivedCommands()[1], IPCCommand::RELOAD);
    EXPECT_EQ(server.receivedCommands()[2], IPCCommand::STATUS);
}

//=============================================================================
// Concurrent Command Tests
//=============================================================================

TEST_F(IPCMultiInstanceTest, ConcurrentCommands) {
    MockIPCServer server(testInstanceId);
    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::atomic<int> successCount{0};
    std::vector<std::thread> senders;

    const int numSenders = 5;
    for (int i = 0; i < numSenders; ++i) {
        senders.emplace_back([this, i, &successCount]() {
            if (sendCommand(testInstanceId, IPCCommand::PING + i)) {
                successCount++;
            }
        });
    }

    for (auto& t : senders) {
        t.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(successCount.load(), numSenders);
    EXPECT_EQ(server.receivedCommands().size(), static_cast<size_t>(numSenders));
}

//=============================================================================
// Error Handling Tests
//=============================================================================

TEST_F(IPCMultiInstanceTest, SendToShuttingDownServer) {
    MockIPCServer server(testInstanceId);
    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Start stopping the server
    std::thread stopThread([&server]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        server.stop();
    });

    // Try to send commands during shutdown (some may fail)
    int successCount = 0;
    for (int i = 0; i < 10; ++i) {
        if (sendCommand(testInstanceId, IPCCommand::PING)) {
            successCount++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    stopThread.join();

    // At least some commands should have succeeded initially
    EXPECT_GT(successCount, 0);
}

TEST_F(IPCMultiInstanceTest, ServerRestartRecovery) {
    // Start first server
    MockIPCServer server1(testInstanceId);
    ASSERT_TRUE(server1.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_TRUE(sendCommand(testInstanceId, IPCCommand::PING));

    // Stop first server
    server1.stop();

    // Verify cannot connect
    EXPECT_FALSE(canConnectTo(testInstanceId));

    // Start new server with same ID
    MockIPCServer server2(testInstanceId);
    ASSERT_TRUE(server2.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Should be able to connect to new server
    EXPECT_TRUE(canConnectTo(testInstanceId));
    EXPECT_TRUE(sendCommand(testInstanceId, IPCCommand::RELOAD));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(server2.receivedCommands().size(), 1u);
}

//=============================================================================
// Socket Permission Tests
//=============================================================================

TEST_F(IPCMultiInstanceTest, SocketFileIsSocket) {
    MockIPCServer server(testInstanceId);
    ASSERT_TRUE(server.start());

    struct stat statBuf;
    ASSERT_EQ(stat(socketPath.c_str(), &statBuf), 0);
    EXPECT_TRUE(S_ISSOCK(statBuf.st_mode)) << "Socket file is not a socket type";
}

//=============================================================================
// Integration with IPCLinux::sendCopyData
//=============================================================================

TEST_F(IPCMultiInstanceTest, SendCopyDataIntegration) {
    // Use a fixed handle that we can match
    const uintptr_t HANDLE_VALUE = 0xABCD1234;
    char pathBuf[128];
    std::snprintf(pathBuf, sizeof(pathBuf), "/tmp/yamy_%p.sock",
                  reinterpret_cast<void*>(HANDLE_VALUE));
    std::string handleSocketPath = pathBuf;

    // Clean up first
    unlink(handleSocketPath.c_str());

    // Create server at the expected path
    int serverSock = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_NE(serverSock, -1);

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, handleSocketPath.c_str(), sizeof(addr.sun_path) - 1);

    ASSERT_EQ(bind(serverSock, (struct sockaddr*)&addr, sizeof(addr)), 0);
    ASSERT_EQ(listen(serverSock, 5), 0);

    // Start receiving thread
    std::atomic<bool> received{false};
    uint32_t receivedId = 0;
    std::thread receiverThread([serverSock, &received, &receivedId]() {
        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        setsockopt(serverSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        int clientSock = accept(serverSock, nullptr, nullptr);
        if (clientSock != -1) {
            uint32_t id, size;
            if (recv(clientSock, &id, sizeof(id), MSG_WAITALL) == sizeof(id) &&
                recv(clientSock, &size, sizeof(size), MSG_WAITALL) == sizeof(size)) {
                receivedId = id;
                received = true;
            }
            close(clientSock);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Use IPCLinux::sendCopyData
    CopyData data;
    data.id = IPCCommand::RELOAD;
    data.size = 0;
    data.data = nullptr;

    WindowHandle target = reinterpret_cast<WindowHandle>(HANDLE_VALUE);
    uintptr_t result = 0;

    bool success = IPCLinux::sendCopyData(nullptr, target, data, 0, 1000, &result);

    receiverThread.join();
    close(serverSock);
    unlink(handleSocketPath.c_str());

    EXPECT_TRUE(success);
    EXPECT_TRUE(received);
    EXPECT_EQ(receivedId, IPCCommand::RELOAD);
}

} // namespace yamy::platform
