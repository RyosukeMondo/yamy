// config_manager.cpp
// Configuration manager implementation

#include "config_manager.h"
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <regex>

namespace fs = std::filesystem;

// Persistence keys
static const std::string KEY_CONFIG_COUNT = "configManager.count";
static const std::string KEY_CONFIG_PREFIX = "configManager.config.";
static const std::string KEY_ACTIVE_INDEX = "configManager.activeIndex";

ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager()
    : m_configStore(nullptr)
    , m_activeIndex(-1)
    , m_changeCallback(nullptr)
{
}

ConfigManager::~ConfigManager()
{
}

void ConfigManager::initialize(ConfigStore* configStore)
{
    Acquire lock(&m_cs);
    m_configStore = configStore;
    load();
    refreshList();
}

std::vector<ConfigEntry> ConfigManager::listConfigs() const
{
    Acquire lock(&m_cs);
    return m_configs;
}

std::string ConfigManager::getActiveConfig() const
{
    Acquire lock(&m_cs);
    if (m_activeIndex >= 0 && m_activeIndex < static_cast<int>(m_configs.size())) {
        return m_configs[m_activeIndex].path;
    }
    return "";
}

int ConfigManager::getActiveIndex() const
{
    Acquire lock(&m_cs);
    return m_activeIndex;
}

bool ConfigManager::setActiveConfig(const std::string& configPath)
{
    Acquire lock(&m_cs);
    int index = findConfig(configPath);
    if (index < 0) {
        return false;
    }

    if (m_activeIndex != index) {
        m_activeIndex = index;
        save();

        if (m_changeCallback) {
            m_changeCallback(configPath);
        }
    }
    return true;
}

bool ConfigManager::setActiveConfig(int index)
{
    Acquire lock(&m_cs);
    if (index < 0 || index >= static_cast<int>(m_configs.size())) {
        return false;
    }

    if (m_activeIndex != index) {
        m_activeIndex = index;
        save();

        if (m_changeCallback && m_activeIndex < static_cast<int>(m_configs.size())) {
            m_changeCallback(m_configs[m_activeIndex].path);
        }
    }
    return true;
}

bool ConfigManager::addConfig(const std::string& configPath)
{
    Acquire lock(&m_cs);

    // Check if already exists
    if (findConfig(configPath) >= 0) {
        return false;
    }

    bool exists = fileExists(configPath);
    std::string name = extractName(configPath);
    m_configs.emplace_back(configPath, name, exists);
    save();
    return true;
}

bool ConfigManager::removeConfig(const std::string& configPath)
{
    Acquire lock(&m_cs);
    int index = findConfig(configPath);
    if (index < 0) {
        return false;
    }

    m_configs.erase(m_configs.begin() + index);

    // Adjust active index if needed
    if (m_activeIndex == index) {
        m_activeIndex = -1;
    } else if (m_activeIndex > index) {
        m_activeIndex--;
    }

    save();
    return true;
}

void ConfigManager::refreshList()
{
    Acquire lock(&m_cs);

    // Update exists status for all entries
    for (auto& entry : m_configs) {
        entry.exists = fileExists(entry.path);
    }

    // Scan default directory
    std::string defaultDir = getDefaultConfigDir();
    if (!defaultDir.empty()) {
        scanDirectory(defaultDir);
    }

    // Also scan current directory for .mayu files
    try {
        fs::path cwd = fs::current_path();
        scanDirectory(cwd.string());
    } catch (const fs::filesystem_error&) {
        // Ignore errors
    }
}

int ConfigManager::scanDirectory(const std::string& directory)
{
    // Note: caller should hold lock
    int added = 0;

    try {
        fs::path dirPath(directory);
        if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
            return 0;
        }

        for (const auto& entry : fs::directory_iterator(dirPath)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string ext = entry.path().extension().string();
            if (ext != ".mayu") {
                continue;
            }

            std::string fullPath = entry.path().string();
            if (findConfig(fullPath) < 0) {
                std::string name = extractName(fullPath);
                m_configs.emplace_back(fullPath, name, true);
                added++;
            }
        }
    } catch (const fs::filesystem_error&) {
        // Ignore filesystem errors
    }

    if (added > 0) {
        save();
    }
    return added;
}

std::string ConfigManager::getDefaultConfigDir()
{
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/.yamy";
    }

    // Windows fallback
    const char* userprofile = std::getenv("USERPROFILE");
    if (userprofile) {
        return std::string(userprofile) + "/.yamy";
    }

    return "";
}

void ConfigManager::setChangeCallback(ConfigChangeCallback callback)
{
    Acquire lock(&m_cs);
    m_changeCallback = callback;
}

void ConfigManager::save() const
{
    if (!m_configStore) {
        return;
    }

    // Save count
    int count = static_cast<int>(m_configs.size());
    m_configStore->write(KEY_CONFIG_COUNT, count);

    // Save each config path
    for (int i = 0; i < count; ++i) {
        std::string key = KEY_CONFIG_PREFIX + std::to_string(i);
        m_configStore->write(key, m_configs[i].path);
    }

    // Save active index
    m_configStore->write(KEY_ACTIVE_INDEX, m_activeIndex);
}

void ConfigManager::load()
{
    if (!m_configStore) {
        return;
    }

    m_configs.clear();

    // Load count
    int count = 0;
    m_configStore->read(KEY_CONFIG_COUNT, &count, 0);

    // Load each config path
    for (int i = 0; i < count; ++i) {
        std::string key = KEY_CONFIG_PREFIX + std::to_string(i);
        std::string path;
        if (m_configStore->read(key, &path) && !path.empty()) {
            bool exists = fileExists(path);
            std::string name = extractName(path);
            m_configs.emplace_back(path, name, exists);
        }
    }

    // Load active index
    m_configStore->read(KEY_ACTIVE_INDEX, &m_activeIndex, -1);

    // Validate active index
    if (m_activeIndex >= static_cast<int>(m_configs.size())) {
        m_activeIndex = -1;
    }
}

std::string ConfigManager::extractName(const std::string& path)
{
    try {
        fs::path p(path);
        return p.stem().string();
    } catch (const std::exception&) {
        // Fallback: find last component manually
        size_t lastSlash = path.find_last_of("/\\");
        size_t start = (lastSlash == std::string::npos) ? 0 : lastSlash + 1;
        size_t dot = path.rfind('.');
        if (dot != std::string::npos && dot > start) {
            return path.substr(start, dot - start);
        }
        return path.substr(start);
    }
}

bool ConfigManager::fileExists(const std::string& path)
{
    try {
        return fs::exists(path) && fs::is_regular_file(path);
    } catch (const fs::filesystem_error&) {
        return false;
    }
}

int ConfigManager::findConfig(const std::string& path) const
{
    for (size_t i = 0; i < m_configs.size(); ++i) {
        if (m_configs[i].path == path) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// ==================== Backup & Restore Implementation ====================

std::string ConfigManager::generateTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::tm tm_now;
#ifdef _WIN32
    localtime_s(&tm_now, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_now);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y%m%d_%H%M%S")
        << "_" << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string ConfigManager::getBackupDir(const std::string& configPath)
{
    try {
        fs::path configFile(configPath);
        fs::path parentDir = configFile.parent_path();

        // Create .backups subdirectory in the same directory as the config
        fs::path backupDir = parentDir / ".backups";
        return backupDir.string();
    } catch (const std::exception&) {
        return "";
    }
}

std::string ConfigManager::createBackup(const std::string& configPath)
{
    Acquire lock(&m_cs);

    if (!fileExists(configPath)) {
        return "";
    }

    try {
        fs::path configFile(configPath);
        std::string backupDirStr = getBackupDir(configPath);
        if (backupDirStr.empty()) {
            return "";
        }

        fs::path backupDir(backupDirStr);

        // Create backup directory if it doesn't exist
        if (!fs::exists(backupDir)) {
            fs::create_directories(backupDir);
        }

        // Generate backup filename: originalname_TIMESTAMP.mayu.bak
        std::string stem = configFile.stem().string();
        std::string timestamp = generateTimestamp();
        std::string backupName = stem + "_" + timestamp + ".mayu.bak";

        fs::path backupPath = backupDir / backupName;

        // Copy the file
        fs::copy_file(configFile, backupPath, fs::copy_options::overwrite_existing);

        // Enforce backup limit
        enforceBackupLimit(configPath);

        return backupPath.string();
    } catch (const fs::filesystem_error&) {
        return "";
    } catch (const std::exception&) {
        return "";
    }
}

std::vector<std::string> ConfigManager::listBackups(const std::string& configPath) const
{
    Acquire lock(&m_cs);
    std::vector<std::string> backups;

    try {
        fs::path configFile(configPath);
        std::string stem = configFile.stem().string();
        std::string backupDirStr = getBackupDir(configPath);

        if (backupDirStr.empty()) {
            return backups;
        }

        fs::path backupDir(backupDirStr);

        if (!fs::exists(backupDir) || !fs::is_directory(backupDir)) {
            return backups;
        }

        // Pattern: stem_YYYYMMDD_HHMMSS_mmm.mayu.bak (mmm = milliseconds)
        std::regex backupPattern(stem + R"(_\d{8}_\d{6}_\d{3}\.mayu\.bak$)");

        for (const auto& entry : fs::directory_iterator(backupDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string filename = entry.path().filename().string();
            if (std::regex_search(filename, backupPattern)) {
                backups.push_back(entry.path().string());
            }
        }

        // Sort by modification time, newest first
        std::sort(backups.begin(), backups.end(), [](const std::string& a, const std::string& b) {
            try {
                auto timeA = fs::last_write_time(a);
                auto timeB = fs::last_write_time(b);
                return timeA > timeB;  // Newer first
            } catch (const fs::filesystem_error&) {
                return a > b;  // Fallback to string comparison
            }
        });

    } catch (const fs::filesystem_error&) {
        // Ignore errors
    } catch (const std::exception&) {
        // Ignore errors
    }

    return backups;
}

std::string ConfigManager::extractOriginalPath(const std::string& backupPath)
{
    try {
        fs::path backup(backupPath);
        fs::path backupDir = backup.parent_path();

        // Backup dir should be named ".backups"
        if (backupDir.filename() != ".backups") {
            return "";
        }

        // Get the config directory (parent of .backups)
        fs::path configDir = backupDir.parent_path();

        // Extract original filename from backup name
        // Pattern: stem_YYYYMMDD_HHMMSS_mmm.mayu.bak -> stem.mayu
        std::string backupName = backup.filename().string();
        std::regex pattern(R"(^(.+)_\d{8}_\d{6}_\d{3}\.mayu\.bak$)");
        std::smatch match;

        if (std::regex_match(backupName, match, pattern) && match.size() > 1) {
            std::string stem = match[1].str();
            fs::path originalPath = configDir / (stem + ".mayu");
            return originalPath.string();
        }
    } catch (const std::exception&) {
        // Ignore errors
    }

    return "";
}

bool ConfigManager::restoreBackup(const std::string& backupPath)
{
    Acquire lock(&m_cs);

    if (!fileExists(backupPath)) {
        return false;
    }

    std::string originalPath = extractOriginalPath(backupPath);
    if (originalPath.empty()) {
        return false;
    }

    try {
        // Create a backup of current state before restoring (if original exists)
        if (fileExists(originalPath)) {
            // Temporarily release lock to avoid deadlock in createBackup
            // Actually, createBackup acquires the lock, so we need to handle this
            // We'll do the backup inline here without calling createBackup
            fs::path configFile(originalPath);
            std::string backupDirStr = getBackupDir(originalPath);
            if (!backupDirStr.empty()) {
                fs::path backupDir(backupDirStr);
                if (!fs::exists(backupDir)) {
                    fs::create_directories(backupDir);
                }
                std::string stem = configFile.stem().string();
                std::string timestamp = generateTimestamp();
                std::string preRestoreBackupName = stem + "_" + timestamp + ".mayu.bak";
                fs::path preRestoreBackupPath = backupDir / preRestoreBackupName;
                fs::copy_file(configFile, preRestoreBackupPath, fs::copy_options::overwrite_existing);
            }
        }

        // Restore from backup
        fs::copy_file(backupPath, originalPath, fs::copy_options::overwrite_existing);

        // Enforce backup limit after creating pre-restore backup
        enforceBackupLimit(originalPath);

        return true;
    } catch (const fs::filesystem_error&) {
        return false;
    } catch (const std::exception&) {
        return false;
    }
}

bool ConfigManager::deleteBackup(const std::string& backupPath)
{
    Acquire lock(&m_cs);

    try {
        fs::path backup(backupPath);

        // Verify it's actually a backup file
        std::string filename = backup.filename().string();
        if (filename.find(".mayu.bak") == std::string::npos) {
            return false;  // Not a backup file
        }

        if (!fs::exists(backup)) {
            return false;
        }

        fs::remove(backup);
        return true;
    } catch (const fs::filesystem_error&) {
        return false;
    } catch (const std::exception&) {
        return false;
    }
}

void ConfigManager::enforceBackupLimit(const std::string& configPath)
{
    // Note: caller should hold lock

    // Get list of backups without acquiring lock (we already hold it)
    std::vector<std::string> backups;

    try {
        fs::path configFile(configPath);
        std::string stem = configFile.stem().string();
        std::string backupDirStr = getBackupDir(configPath);

        if (backupDirStr.empty()) {
            return;
        }

        fs::path backupDir(backupDirStr);

        if (!fs::exists(backupDir) || !fs::is_directory(backupDir)) {
            return;
        }

        std::regex backupPattern(stem + R"(_\d{8}_\d{6}_\d{3}\.mayu\.bak$)");

        for (const auto& entry : fs::directory_iterator(backupDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string filename = entry.path().filename().string();
            if (std::regex_search(filename, backupPattern)) {
                backups.push_back(entry.path().string());
            }
        }

        // Sort by modification time, oldest first (for deletion)
        std::sort(backups.begin(), backups.end(), [](const std::string& a, const std::string& b) {
            try {
                auto timeA = fs::last_write_time(a);
                auto timeB = fs::last_write_time(b);
                return timeA < timeB;  // Older first
            } catch (const fs::filesystem_error&) {
                return a < b;
            }
        });

        // Delete oldest backups if we exceed the limit
        while (backups.size() > static_cast<size_t>(MAX_BACKUPS_PER_CONFIG)) {
            try {
                fs::remove(backups.front());
                backups.erase(backups.begin());
            } catch (const fs::filesystem_error&) {
                break;  // Stop if we can't delete
            }
        }

    } catch (const fs::filesystem_error&) {
        // Ignore errors
    } catch (const std::exception&) {
        // Ignore errors
    }
}
