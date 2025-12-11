#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ipc_control_server.h - IPC server for yamy-ctl control commands
//
// Listens on a Unix domain socket for control commands from yamy-ctl.
// Handles: reload, stop, start, status commands.
//

#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <memory>

namespace yamy::platform {

/// Control command types (must match yamy_ctl.cpp)
enum class ControlCommand {
    Reload,
    Stop,
    Start,
    GetStatus,
    GetConfig,
    GetKeymaps,
    GetMetrics
};

/// Result of command execution
struct ControlResult {
    bool success;
    std::string message;
};

/// Callback for handling control commands
/// @param cmd The command received
/// @param data Optional data (e.g., config name for reload)
/// @return Result of command execution
using ControlCommandCallback = std::function<ControlResult(ControlCommand cmd, const std::string& data)>;

/// IPC Control Server
/// Listens for control commands from yamy-ctl and dispatches to registered callback.
class IPCControlServer {
public:
    /// Default socket path
    static constexpr const char* DEFAULT_SOCKET_PATH = "/tmp/yamy-engine.sock";

    /// Construct server with optional custom socket path
    /// @param socketPath Path for the Unix domain socket
    explicit IPCControlServer(const std::string& socketPath = DEFAULT_SOCKET_PATH);

    /// Destructor - stops server if running
    ~IPCControlServer();

    // Non-copyable, non-movable
    IPCControlServer(const IPCControlServer&) = delete;
    IPCControlServer& operator=(const IPCControlServer&) = delete;

    /// Register callback for handling commands
    /// @param callback Function to call when command is received
    void setCommandCallback(ControlCommandCallback callback);

    /// Start listening for connections (non-blocking, spawns thread)
    /// @return true if server started successfully
    bool start();

    /// Stop the server
    void stop();

    /// Check if server is running
    bool isRunning() const;

    /// Get the socket path being used
    const std::string& socketPath() const { return m_socketPath; }

private:
    /// Server thread main loop
    void serverLoop();

    /// Handle a single client connection
    /// @param clientFd Client socket file descriptor
    void handleClient(int clientFd);

    std::string m_socketPath;
    int m_serverFd;
    std::atomic<bool> m_running;
    std::unique_ptr<std::thread> m_serverThread;
    ControlCommandCallback m_callback;
};

} // namespace yamy::platform
