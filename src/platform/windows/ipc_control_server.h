#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ipc_control_server.h - IPC server stub for Windows
//
// Windows implementation currently stubbed.
//

#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <memory>

namespace yamy {
namespace platform {

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
using ControlCommandCallback = std::function<ControlResult(ControlCommand cmd, const std::string& data)>;

/// IPC Control Server (Windows Stub)
class IPCControlServer {
public:
    /// Default socket path (Not used on Windows)
    static constexpr const char* DEFAULT_SOCKET_PATH = "";

    /// Construct server
    explicit IPCControlServer(const std::string& socketPath = DEFAULT_SOCKET_PATH);

    /// Destructor
    ~IPCControlServer();

    // Non-copyable, non-movable
    IPCControlServer(const IPCControlServer&) = delete;
    IPCControlServer& operator=(const IPCControlServer&) = delete;

    /// Register callback for handling commands
    void setCommandCallback(ControlCommandCallback callback);

    /// Start listening (Stub)
    bool start();

    /// Stop the server (Stub)
    void stop();

    /// Check if server is running
    bool isRunning() const;

    /// Get socket path
    const std::string& socketPath() const { return m_socketPath; }

private:
    std::string m_socketPath;
    bool m_running;
    ControlCommandCallback m_callback;
};

} // namespace platform
} // namespace yamy
