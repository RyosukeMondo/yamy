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
#include <set>
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

    // ==================== Backup & Restore ====================

    /// Create a backup of a configuration file
    /// @param configPath Path to the configuration file to backup
    /// @return Path to the created backup file, or empty string on failure
    std::string createBackup(const std::string& configPath);

    /// List all backups for a configuration file
    /// @param configPath Path to the configuration file
    /// @return Vector of backup file paths, sorted newest first
    std::vector<std::string> listBackups(const std::string& configPath) const;

    /// Restore a configuration from a backup
    /// @param backupPath Path to the backup file
    /// @return true if restore succeeded
    bool restoreBackup(const std::string& backupPath);

    /// Delete a backup file
    /// @param backupPath Path to the backup file to delete
    /// @return true if deletion succeeded
    bool deleteBackup(const std::string& backupPath);

    /// Get the backup directory for a config file
    /// @param configPath Path to the configuration file
    /// @return Path to the backup directory
    static std::string getBackupDir(const std::string& configPath);

    /// Maximum number of backups to keep per configuration
    static constexpr int MAX_BACKUPS_PER_CONFIG = 10;

    // ==================== Import/Export ====================

    /// Result of an import or export operation
    struct ImportExportResult {
        bool success;
        std::string errorMessage;
        std::vector<std::string> filesProcessed;

        ImportExportResult() : success(false) {}
        ImportExportResult(bool s) : success(s) {}
        ImportExportResult(bool s, const std::string& err) : success(s), errorMessage(err) {}
    };

    /// Export a configuration and all its dependencies to an archive
    /// @param configPath Path to the main configuration file
    /// @param archivePath Path where the archive will be created
    /// @return Result with success status and list of exported files
    ImportExportResult exportConfig(const std::string& configPath,
                                    const std::string& archivePath);

    /// Import a configuration from an archive
    /// @param archivePath Path to the archive file
    /// @param targetDir Directory where the config will be extracted
    /// @param overwrite If true, overwrite existing files without confirmation
    /// @return Result with success status and list of imported files
    ImportExportResult importConfig(const std::string& archivePath,
                                    const std::string& targetDir,
                                    bool overwrite = false);

    /// List contents of an archive without extracting
    /// @param archivePath Path to the archive file
    /// @return Vector of file paths contained in the archive
    std::vector<std::string> listArchiveContents(const std::string& archivePath) const;

    /// Validate an archive before importing
    /// @param archivePath Path to the archive file
    /// @return true if archive is valid and can be imported
    bool validateArchive(const std::string& archivePath) const;

    /// Get the default export directory
    static std::string getExportDir();

    /// Archive file extension
    static constexpr const char* ARCHIVE_EXTENSION = ".yamy-pkg";

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

    /// Extract original config path from a backup path
    static std::string extractOriginalPath(const std::string& backupPath);

    /// Enforce backup limit by deleting oldest backups
    void enforceBackupLimit(const std::string& configPath);

    /// Generate timestamp string for backup filenames
    static std::string generateTimestamp();

    /// Recursively find all include dependencies of a config file
    /// @param configPath Path to the config file
    /// @param basePath Base path for resolving relative includes
    /// @param dependencies Output set of all dependency paths
    /// @param visited Set of already visited files (to avoid cycles)
    static void findDependencies(const std::string& configPath,
                                 const std::string& basePath,
                                 std::set<std::string>& dependencies,
                                 std::set<std::string>& visited);

    /// Parse a config file and extract include directives
    /// @param configPath Path to the config file
    /// @return Vector of include paths (as written in the file)
    static std::vector<std::string> parseIncludes(const std::string& configPath);

    /// Resolve an include path relative to a base path
    /// @param includePath The include path as written in the file
    /// @param basePath The base path (directory of the including file)
    /// @return Resolved absolute path, or empty string if not found
    static std::string resolveIncludePath(const std::string& includePath,
                                          const std::string& basePath);

    /// Write archive header
    static bool writeArchiveHeader(std::ofstream& out, uint32_t fileCount);

    /// Write a file entry to the archive
    static bool writeArchiveEntry(std::ofstream& out,
                                  const std::string& relativePath,
                                  const std::string& absolutePath);

    /// Read archive header
    static bool readArchiveHeader(std::ifstream& in, uint32_t& fileCount);

    /// Read a file entry from the archive
    static bool readArchiveEntry(std::ifstream& in,
                                 std::string& relativePath,
                                 std::vector<char>& content);

    /// Archive magic number for validation
    static constexpr uint32_t ARCHIVE_MAGIC = 0x59414D59;  // "YAMY"

    /// Archive version
    static constexpr uint32_t ARCHIVE_VERSION = 1;

    mutable CriticalSection m_cs;       /// Thread synchronization
    ConfigStore* m_configStore;         /// Persistent storage (not owned)
    std::vector<ConfigEntry> m_configs; /// List of known configurations
    int m_activeIndex;                  /// Index of active config (-1 if none)
    ConfigChangeCallback m_changeCallback; /// Notification callback
};

#endif // !_CONFIG_MANAGER_H
