#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// config_manager.h
// Manages multiple configuration files (.mayu) for yamy

#ifndef _CONFIG_MANAGER_H
#define _CONFIG_MANAGER_H

#include "multithread.h"
#include "../utils/config_store.h"
#include <string>
#include <vector>
#include <functional>

/// Configuration file entry with path and optional metadata
struct ConfigEntry {
    std::string path;           /// Full path to .mayu file
    std::string name;           /// Display name (basename without extension)
    bool exists;                /// Whether the file currently exists on disk

    ConfigEntry() : exists(false) {}
    ConfigEntry(const std::string& p, const std::string& n, bool e)
        : path(p), name(n), exists(e) {}
};

/// Callback type for configuration change notifications
using ConfigChangeCallback = std::function<void(const std::string& newConfigPath)>;

/// Manages list of available configurations and tracks the active one
/// Thread-safe singleton with persistence via ConfigStore
class ConfigManager
{
public:
    /// Get singleton instance
    static ConfigManager& instance();

    /// Initialize with config store for persistence (call once at startup)
    void initialize(ConfigStore* configStore);

    /// Get list of all known configurations
    std::vector<ConfigEntry> listConfigs() const;

    /// Get path to active configuration (empty if none)
    std::string getActiveConfig() const;

    /// Get index of active configuration (-1 if none)
    int getActiveIndex() const;

    /// Set active configuration by path
    /// @return true if path exists in list and was set
    bool setActiveConfig(const std::string& configPath);

    /// Set active configuration by index
    /// @return true if index valid and was set
    bool setActiveConfig(int index);

    /// Add a configuration path to the list
    /// @return true if added (not already present)
    bool addConfig(const std::string& configPath);

    /// Remove a configuration from the list
    /// @return true if found and removed
    bool removeConfig(const std::string& configPath);

    /// Refresh the list by scanning known directories for .mayu files
    /// Also validates that existing entries still exist on disk
    void refreshList();

    /// Scan a directory for .mayu files and add them
    /// @return number of new configs added
    int scanDirectory(const std::string& directory);

    /// Get the default config directory (~/.yamy/)
    static std::string getDefaultConfigDir();

    /// Register callback for active config changes
    void setChangeCallback(ConfigChangeCallback callback);

    /// Save current state to persistent storage
    void save() const;

    /// Load state from persistent storage
    void load();

private:
    ConfigManager();
    ~ConfigManager();

    // Non-copyable
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    /// Extract display name from path
    static std::string extractName(const std::string& path);

    /// Check if file exists on disk
    static bool fileExists(const std::string& path);

    /// Find index of path in list (-1 if not found)
    int findConfig(const std::string& path) const;

    mutable CriticalSection m_cs;       /// Thread synchronization
    ConfigStore* m_configStore;         /// Persistent storage (not owned)
    std::vector<ConfigEntry> m_configs; /// List of known configurations
    int m_activeIndex;                  /// Index of active config (-1 if none)
    ConfigChangeCallback m_changeCallback; /// Notification callback
};

#endif // !_CONFIG_MANAGER_H
