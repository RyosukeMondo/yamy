#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// config_metadata.h
// Stores and manages metadata for configuration files

#ifndef _CONFIG_METADATA_H
#define _CONFIG_METADATA_H

#include <string>
#include <vector>
#include <ctime>

/// Stores metadata about a configuration file
/// Metadata is persisted as JSON in ~/.yamy/.metadata/ directory
struct ConfigMetadataInfo {
    std::string name;           /// Display name for the config
    std::string description;    /// User description of this config
    std::string author;         /// Author name
    std::time_t createdDate;    /// When the config was first created
    std::time_t modifiedDate;   /// When the config was last modified
    std::vector<std::string> tags; /// User-defined tags for organization

    ConfigMetadataInfo();
};

/// Manages metadata storage for configuration files
/// Metadata is stored separately from .mayu files in JSON format
/// Operations are designed to be optional and fail gracefully
class ConfigMetadata
{
public:
    ConfigMetadata();
    ~ConfigMetadata();

    /// Load metadata for a config file
    /// @param configPath Path to the .mayu configuration file
    /// @return true if loaded successfully, false if no metadata exists
    bool load(const std::string& configPath);

    /// Save metadata for a config file
    /// @param configPath Path to the .mayu configuration file
    /// @return true if saved successfully
    bool save(const std::string& configPath);

    /// Update the modification timestamp and save
    /// @param configPath Path to the .mayu configuration file
    /// @return true if saved successfully
    bool touch(const std::string& configPath);

    /// Delete metadata for a config file
    /// @param configPath Path to the .mayu configuration file
    /// @return true if deleted successfully or didn't exist
    bool remove(const std::string& configPath);

    /// Check if metadata exists for a config file
    /// @param configPath Path to the .mayu configuration file
    /// @return true if metadata file exists
    static bool exists(const std::string& configPath);

    /// Get the metadata file path for a config path
    /// @param configPath Path to the .mayu configuration file
    /// @return Path to the metadata JSON file
    static std::string getMetadataPath(const std::string& configPath);

    /// Get the metadata directory (~/.yamy/.metadata/)
    /// @return Path to metadata directory
    static std::string getMetadataDir();

    /// Ensure the metadata directory exists
    /// @return true if directory exists or was created
    static bool ensureMetadataDirExists();

    // Accessors
    const ConfigMetadataInfo& info() const { return m_info; }
    ConfigMetadataInfo& info() { return m_info; }

    // Convenience setters that auto-update modifiedDate
    void setName(const std::string& name);
    void setDescription(const std::string& description);
    void setAuthor(const std::string& author);
    void addTag(const std::string& tag);
    void removeTag(const std::string& tag);
    void clearTags();

private:
    /// Parse JSON content into metadata info
    bool parseJson(const std::string& jsonContent);

    /// Serialize metadata info to JSON
    std::string toJson() const;

    ConfigMetadataInfo m_info;
};

#endif // !_CONFIG_METADATA_H
