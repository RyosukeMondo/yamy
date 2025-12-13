#pragma once

#include <string>
#include <thread>
#include <memory>
#include <functional>
#include "core/platform/ipc_defs.h"

// Forward declaration of real Engine
class Engine;

/// EngineAdapter - Bridges Qt GUI and the real keyboard remapping Engine
///
/// This adapter provides a simplified interface matching the stub Engine's API
/// while delegating to the real Engine implementation. It manages the engine's
/// lifecycle including thread management and configuration loading.
class EngineAdapter {
public:
    /// Constructor - Takes ownership of the Engine pointer
    /// @param engine Pointer to the real Engine instance (adapter takes ownership)
    explicit EngineAdapter(Engine* engine);

    /// Destructor - Ensures clean thread shutdown
    ~EngineAdapter();

    // Prevent copying
    EngineAdapter(const EngineAdapter&) = delete;
    EngineAdapter& operator=(const EngineAdapter&) = delete;

    /// Get enabled state
    /// @return true if engine is enabled, false otherwise
    bool getIsEnabled() const;

    /// Check if engine thread is running
    /// @return true if engine thread is active, false otherwise
    bool isRunning() const;

    /// Enable the engine (allows key processing)
    void enable();

    /// Disable the engine (stops key processing but keeps thread running)
    void disable();

    /// Start the engine thread
    void start();

    /// Stop the engine thread and wait for it to complete
    void stop();

    /// Load configuration from a .mayu file
    /// @param path Path to the .mayu configuration file
    /// @return true if configuration loaded successfully, false on error
    bool loadConfig(const std::string& path);

    /// Get current configuration file path
    /// @return Path to the currently loaded configuration file
    const std::string& getConfigPath() const;

    /// Get total number of keys processed
    /// @return Key count
    uint64_t keyCount() const;

    /// Get engine status as JSON string
    /// Format: {"state": "running/stopped", "uptime": seconds, "config": "name",
    ///          "key_count": N, "current_keymap": "name"}
    /// @return JSON string with engine status
    std::string getStatusJson() const;

    /// Get configuration information as JSON string
    /// Format: {"config_path": "path", "config_name": "name", "loaded_time": "ISO8601"}
    /// @return JSON string with config information
    std::string getConfigJson() const;

    /// Get keymaps as JSON string
    /// Format: {"keymaps": [{"name": "name", "window_class": "regex",
    ///                       "window_title": "regex"}, ...]}
    /// @return JSON string with keymaps list
    std::string getKeymapsJson() const;

    /// Get performance metrics as JSON string
    /// Format: {"latency_avg_ns": N, "latency_p99_ns": N, "latency_max_ns": N,
    ///          "cpu_usage_percent": N, "keys_per_second": N}
    /// @return JSON string with performance metrics
    std::string getMetricsJson() const;

    /// Callback type for engine notifications
    /// @param type Notification message type (ConfigLoaded, ConfigError, etc.)
    /// @param data Additional data associated with the notification (e.g., error message)
    using NotificationCallback = std::function<void(yamy::MessageType type, const std::string& data)>;

    /// Set notification callback for engine events
    /// @param callback Callback function to be invoked on engine notifications
    void setNotificationCallback(NotificationCallback callback);

private:
    Engine* m_engine;                           ///< Real Engine instance (owned)
    std::string m_configPath;                   ///< Path to loaded configuration
    std::thread m_engineThread;                 ///< Thread running the engine
    std::chrono::steady_clock::time_point m_startTime;  ///< Time when engine was started
    std::chrono::system_clock::time_point m_configLoadedTime; ///< Time when config was loaded
    NotificationCallback m_notificationCallback; ///< Callback for engine notifications
};
