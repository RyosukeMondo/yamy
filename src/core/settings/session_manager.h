#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// session_manager.h
// Manages session state persistence for yamy

#ifndef _SESSION_MANAGER_H
#define _SESSION_MANAGER_H

#include <string>
#include <map>

namespace yamy {

/// Window position data for session restoration
struct WindowPosition {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool valid = false;
};

/// Session data persisted between application runs
struct SessionData {
    std::string activeConfigPath;       /// Path to the active configuration file
    bool engineWasRunning = false;      /// Whether engine was running on shutdown
    std::map<std::string, WindowPosition> windowPositions; /// Dialog window positions
    int64_t savedTimestamp = 0;         /// When session was saved (unix timestamp)
};

/// Manages session state persistence
/// Saves and restores: active config, engine state, window positions
/// Data stored in JSON format at ~/.config/yamy/session.json
class SessionManager {
public:
    /// Get singleton instance
    static SessionManager& instance();

    /// Save current session state to disk
    /// @return true if save succeeded
    bool saveSession();

    /// Restore session state from disk
    /// @return true if restore succeeded (false if no session or corrupt)
    bool restoreSession();

    /// Check if a saved session exists
    /// @return true if session file exists and is readable
    bool hasSession() const;

    /// Clear saved session data
    /// @return true if cleared successfully
    bool clearSession();

    // Session data accessors
    const SessionData& data() const { return m_data; }
    SessionData& data() { return m_data; }

    /// Set the active config path
    void setActiveConfig(const std::string& configPath);

    /// Set engine running state
    void setEngineRunning(bool running);

    /// Save window position for a named window
    void saveWindowPosition(const std::string& windowName,
                            int x, int y, int width, int height);

    /// Get window position for a named window
    /// @return WindowPosition with valid=true if found, valid=false otherwise
    WindowPosition getWindowPosition(const std::string& windowName) const;

    /// Get the session file path
    /// @return Path to ~/.config/yamy/session.json
    static std::string getSessionPath();

    /// Get the config directory path
    /// @return Path to ~/.config/yamy/
    static std::string getConfigDir();

    /// Get the autostart directory path
    /// @return Path to $XDG_CONFIG_HOME/autostart or ~/.config/autostart
    static std::string getAutoStartPath();

    /// Get the autostart desktop file path
    /// @return Path to autostart/yamy.desktop
    static std::string getAutoStartFilePath();

    /// Enable application autostart on login
    /// Creates yamy.desktop file in XDG autostart directory
    /// @return true if autostart was enabled successfully
    bool enableAutoStart();

    /// Disable application autostart on login
    /// Removes yamy.desktop file from XDG autostart directory
    /// @return true if autostart was disabled successfully
    bool disableAutoStart();

    /// Check if autostart is currently enabled
    /// @return true if yamy.desktop exists and is valid in autostart directory
    bool isAutoStartEnabled() const;

private:
    SessionManager();
    ~SessionManager();

    // Non-copyable
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    /// Ensure config directory exists
    bool ensureConfigDirExists() const;

    /// Parse JSON content into session data
    bool parseJson(const std::string& jsonContent);

    /// Serialize session data to JSON
    std::string toJson() const;

    /// Validate session data after loading
    bool validateSession() const;

    SessionData m_data;
};

}  // namespace yamy

#endif // !_SESSION_MANAGER_H
