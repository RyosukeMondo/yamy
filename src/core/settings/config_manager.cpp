// config_manager.cpp
// Configuration manager implementation

#include "config_manager.h"
#include "config_metadata.h"
#include "platform_time.h"
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <regex>
#include <fstream>
#include <cstring>
#include <unordered_map>

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
#ifndef _WIN32
    , m_configWatcher(std::make_unique<ConfigWatcher>())
#else
    , m_configWatcher(nullptr)  // ConfigWatcher not available on Windows
#endif
{
#ifndef _WIN32
    // Set up the callback from the watcher to the manager (Linux only)
    if (m_configWatcher) {
        m_configWatcher->setChangeCallback([this](const std::string& path) {
            this->onActiveConfigChanged(path);
        });
    }
#endif
}

ConfigManager::~ConfigManager()
{
}

void ConfigManager::setAutoReloadEnabled(bool enabled)
{
    if (m_configWatcher) {
        m_configWatcher->setAutoReloadEnabled(enabled);
    }
}

void ConfigManager::initialize(ConfigStore* configStore)
{
    Acquire lock(&m_cs);
    m_configStore = configStore;
    load();
    refreshList();

    // Start watching the active config, if any
    std::string activeConfig = getActiveConfig();
    if (!activeConfig.empty() && m_configWatcher) {
        m_configWatcher->setConfigPath(activeConfig);
        m_configWatcher->start();
    }
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

        if (m_configWatcher) {
            m_configWatcher->setConfigPath(configPath);
        }

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

        const std::string& newPath = m_configs[m_activeIndex].path;
        if (m_configWatcher) {
            m_configWatcher->setConfigPath(newPath);
        }

        if (m_changeCallback) {
            m_changeCallback(newPath);
        }
    }
    return true;
}

bool ConfigManager::setNextConfig()
{
    Acquire lock(&m_cs);

    // Need at least one config to switch
    if (m_configs.empty()) {
        return false;
    }

    // If only one config, nothing to cycle
    if (m_configs.size() == 1) {
        // If not already active, make it active
        if (m_activeIndex != 0) {
            m_activeIndex = 0;
            save();
            if (m_changeCallback) {
                m_changeCallback(m_configs[0].path);
            }
        }
        return true;
    }

    // Calculate next index with wrap-around
    int nextIndex = (m_activeIndex < 0) ? 0 : (m_activeIndex + 1) % static_cast<int>(m_configs.size());

    // Skip non-existent configs
    int startIndex = nextIndex;
    while (!m_configs[nextIndex].exists) {
        nextIndex = (nextIndex + 1) % static_cast<int>(m_configs.size());
        if (nextIndex == startIndex) {
            // All configs don't exist, give up
            return false;
        }
    }

    if (m_activeIndex != nextIndex) {
        m_activeIndex = nextIndex;
        save();

        if (m_changeCallback) {
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

void ConfigManager::onActiveConfigChanged(const std::string& path)
{
    Acquire lock(&m_cs);

    // Verify the changed file is still the active one
    std::string activeConfig = getActiveConfig();
    if (path != activeConfig) {
        return;
    }

    // Trigger reload
    if (m_changeCallback) {
        m_changeCallback(path);
    }
}

// ==================== Backup & Restore Implementation ====================

std::string ConfigManager::generateTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::tm tm_now;
    yamy::platform::localtime_safe(&time_t_now, &tm_now);

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

// ==================== Import/Export Implementation ====================

std::string ConfigManager::getExportDir()
{
    std::string configDir = getDefaultConfigDir();
    if (configDir.empty()) {
        return "";
    }
    return configDir + "/exports";
}

std::vector<std::string> ConfigManager::parseIncludes(const std::string& configPath)
{
    std::vector<std::string> includes;

    std::ifstream file(configPath);
    if (!file.is_open()) {
        return includes;
    }

    std::string line;
    std::regex includeRegex(R"(^\s*include\s+\"?([^\"]+)\"?\s*$)");
    std::regex includeRegex2(R"(^\s*include\s+(\S+)\s*$)");

    while (std::getline(file, line)) {
        // Skip comments
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        std::smatch match;
        if (std::regex_match(line, match, includeRegex) && match.size() > 1) {
            includes.push_back(match[1].str());
        } else if (std::regex_match(line, match, includeRegex2) && match.size() > 1) {
            includes.push_back(match[1].str());
        }
    }

    return includes;
}

std::string ConfigManager::resolveIncludePath(const std::string& includePath,
                                              const std::string& basePath)
{
    // If absolute path, use it directly
    fs::path incPath(includePath);
    if (incPath.is_absolute()) {
        if (fs::exists(incPath)) {
            return incPath.string();
        }
        return "";
    }

    // Try relative to base path
    fs::path base(basePath);
    fs::path resolved = base / includePath;
    if (fs::exists(resolved)) {
        return fs::canonical(resolved).string();
    }

    // Try in the default config directory
    std::string defaultDir = getDefaultConfigDir();
    if (!defaultDir.empty()) {
        resolved = fs::path(defaultDir) / includePath;
        if (fs::exists(resolved)) {
            return fs::canonical(resolved).string();
        }
    }

    return "";
}

void ConfigManager::findDependencies(const std::string& configPath,
                                     const std::string& basePath,
                                     std::set<std::string>& dependencies,
                                     std::set<std::string>& visited)
{
    // Avoid cycles
    std::string canonical;
    try {
        canonical = fs::canonical(configPath).string();
    } catch (const fs::filesystem_error&) {
        return;
    }

    if (visited.find(canonical) != visited.end()) {
        return;
    }
    visited.insert(canonical);

    // Parse includes from this file
    std::vector<std::string> includes = parseIncludes(configPath);

    // Get the directory of the current file for relative path resolution
    fs::path configDir = fs::path(configPath).parent_path();

    for (const auto& inc : includes) {
        std::string resolved = resolveIncludePath(inc, configDir.string());
        if (!resolved.empty()) {
            dependencies.insert(resolved);
            // Recursively find dependencies of included files
            findDependencies(resolved, fs::path(resolved).parent_path().string(),
                           dependencies, visited);
        }
    }
}

bool ConfigManager::writeArchiveHeader(std::ofstream& out, uint32_t fileCount)
{
    // Write magic number
    out.write(reinterpret_cast<const char*>(&ARCHIVE_MAGIC), sizeof(ARCHIVE_MAGIC));

    // Write version
    out.write(reinterpret_cast<const char*>(&ARCHIVE_VERSION), sizeof(ARCHIVE_VERSION));

    // Write file count
    out.write(reinterpret_cast<const char*>(&fileCount), sizeof(fileCount));

    return out.good();
}

bool ConfigManager::writeArchiveEntry(std::ofstream& out,
                                      const std::string& relativePath,
                                      const std::string& absolutePath)
{
    // Read file content
    std::ifstream file(absolutePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> content(static_cast<size_t>(fileSize));
    if (fileSize > 0 && !file.read(content.data(), fileSize)) {
        return false;
    }

    // Write path length and path
    uint32_t pathLen = static_cast<uint32_t>(relativePath.size());
    out.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
    out.write(relativePath.c_str(), pathLen);

    // Write content length and content
    uint64_t contentLen = static_cast<uint64_t>(fileSize);
    out.write(reinterpret_cast<const char*>(&contentLen), sizeof(contentLen));
    if (fileSize > 0) {
        out.write(content.data(), fileSize);
    }

    return out.good();
}

bool ConfigManager::readArchiveHeader(std::ifstream& in, uint32_t& fileCount)
{
    uint32_t magic = 0;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != ARCHIVE_MAGIC) {
        return false;
    }

    uint32_t version = 0;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != ARCHIVE_VERSION) {
        return false;
    }

    in.read(reinterpret_cast<char*>(&fileCount), sizeof(fileCount));

    return in.good();
}

bool ConfigManager::readArchiveEntry(std::ifstream& in,
                                     std::string& relativePath,
                                     std::vector<char>& content)
{
    // Read path length and path
    uint32_t pathLen = 0;
    in.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));
    if (!in.good() || pathLen > 4096) {  // Sanity check
        return false;
    }

    relativePath.resize(pathLen);
    in.read(&relativePath[0], pathLen);

    // Read content length and content
    uint64_t contentLen = 0;
    in.read(reinterpret_cast<char*>(&contentLen), sizeof(contentLen));
    if (!in.good() || contentLen > 100 * 1024 * 1024) {  // 100MB limit
        return false;
    }

    content.resize(static_cast<size_t>(contentLen));
    if (contentLen > 0) {
        in.read(content.data(), static_cast<std::streamsize>(contentLen));
    }

    return in.good();
}

ConfigManager::ImportExportResult ConfigManager::exportConfig(
    const std::string& configPath,
    const std::string& archivePath)
{
    Acquire lock(&m_cs);

    ImportExportResult result;

    // Validate input file exists
    if (!fileExists(configPath)) {
        result.errorMessage = "Configuration file does not exist: " + configPath;
        return result;
    }

    try {
        // Get canonical path of the main config
        fs::path mainConfig = fs::canonical(configPath);
        fs::path mainDir = mainConfig.parent_path();

        // Find all dependencies
        std::set<std::string> dependencies;
        std::set<std::string> visited;
        findDependencies(configPath, mainDir.string(), dependencies, visited);

        // Add the main config file to the list
        dependencies.insert(mainConfig.string());

        // Calculate relative paths from the main config's directory
        std::vector<std::pair<std::string, std::string>> filesToArchive;
        for (const auto& dep : dependencies) {
            fs::path depPath(dep);
            fs::path relative;

            // Try to make path relative to main config directory
            try {
                relative = fs::relative(depPath, mainDir);
            } catch (const fs::filesystem_error&) {
                // If relative fails, use filename only
                relative = depPath.filename();
            }

            // Ensure relative path doesn't escape (no ..)
            std::string relStr = relative.string();
            if (relStr.find("..") != std::string::npos) {
                // Use just the filename for paths outside the main directory
                relStr = depPath.filename().string();
            }

            filesToArchive.emplace_back(relStr, dep);
        }

        // Create archive directory if needed
        fs::path archiveDir = fs::path(archivePath).parent_path();
        if (!archiveDir.empty() && !fs::exists(archiveDir)) {
            fs::create_directories(archiveDir);
        }

        // Write archive
        std::ofstream archive(archivePath, std::ios::binary);
        if (!archive.is_open()) {
            result.errorMessage = "Cannot create archive file: " + archivePath;
            return result;
        }

        // Write header
        if (!writeArchiveHeader(archive, static_cast<uint32_t>(filesToArchive.size()))) {
            result.errorMessage = "Failed to write archive header";
            return result;
        }

        // Write each file
        for (const auto& [relativePath, absolutePath] : filesToArchive) {
            if (!writeArchiveEntry(archive, relativePath, absolutePath)) {
                result.errorMessage = "Failed to write file to archive: " + relativePath;
                return result;
            }
            result.filesProcessed.push_back(relativePath);
        }

        archive.close();
        result.success = true;

    } catch (const fs::filesystem_error& e) {
        result.errorMessage = std::string("Filesystem error: ") + e.what();
    } catch (const std::exception& e) {
        result.errorMessage = std::string("Error: ") + e.what();
    }

    return result;
}

ConfigManager::ImportExportResult ConfigManager::importConfig(
    const std::string& archivePath,
    const std::string& targetDir,
    bool overwrite)
{
    Acquire lock(&m_cs);

    ImportExportResult result;

    // Validate archive exists
    if (!fileExists(archivePath)) {
        result.errorMessage = "Archive file does not exist: " + archivePath;
        return result;
    }

    try {
        std::ifstream archive(archivePath, std::ios::binary);
        if (!archive.is_open()) {
            result.errorMessage = "Cannot open archive file: " + archivePath;
            return result;
        }

        // Read and validate header
        uint32_t fileCount = 0;
        if (!readArchiveHeader(archive, fileCount)) {
            result.errorMessage = "Invalid archive format or version";
            return result;
        }

        // Sanity check file count
        if (fileCount > 1000) {
            result.errorMessage = "Archive contains too many files";
            return result;
        }

        // Create target directory if needed
        fs::path targetPath(targetDir);
        if (!fs::exists(targetPath)) {
            fs::create_directories(targetPath);
        }

        // First pass: check for existing files if overwrite is false
        if (!overwrite) {
            std::streampos savedPos = archive.tellg();
            for (uint32_t i = 0; i < fileCount; ++i) {
                std::string relativePath;
                std::vector<char> content;
                if (!readArchiveEntry(archive, relativePath, content)) {
                    result.errorMessage = "Failed to read archive entry";
                    return result;
                }

                fs::path destPath = targetPath / relativePath;
                if (fs::exists(destPath)) {
                    result.errorMessage = "File already exists (use overwrite): " +
                                         destPath.string();
                    return result;
                }
            }
            archive.seekg(savedPos);
        }

        // Re-read header for extraction
        archive.seekg(0);
        if (!readArchiveHeader(archive, fileCount)) {
            result.errorMessage = "Failed to re-read archive header";
            return result;
        }

        // Extract files
        for (uint32_t i = 0; i < fileCount; ++i) {
            std::string relativePath;
            std::vector<char> content;
            if (!readArchiveEntry(archive, relativePath, content)) {
                result.errorMessage = "Failed to read archive entry " + std::to_string(i);
                return result;
            }

            // Validate relative path (no path traversal)
            if (relativePath.find("..") != std::string::npos ||
                relativePath[0] == '/' || relativePath[0] == '\\') {
                result.errorMessage = "Invalid path in archive: " + relativePath;
                return result;
            }

            fs::path destPath = targetPath / relativePath;

            // Create parent directories
            fs::path parentDir = destPath.parent_path();
            if (!parentDir.empty() && !fs::exists(parentDir)) {
                fs::create_directories(parentDir);
            }

            // Write file
            std::ofstream outFile(destPath, std::ios::binary);
            if (!outFile.is_open()) {
                result.errorMessage = "Cannot create file: " + destPath.string();
                return result;
            }

            if (!content.empty()) {
                outFile.write(content.data(), static_cast<std::streamsize>(content.size()));
            }
            outFile.close();

            result.filesProcessed.push_back(destPath.string());
        }

        result.success = true;

        // Add the main config file to the manager if it's a .mayu file
        for (const auto& file : result.filesProcessed) {
            if (file.size() > 5 && file.substr(file.size() - 5) == ".mayu") {
                // Temporarily release lock to avoid deadlock
                // Actually we're inside the lock, so just add directly
                int index = findConfig(file);
                if (index < 0) {
                    std::string name = extractName(file);
                    m_configs.emplace_back(file, name, true);
                }
            }
        }

        if (!result.filesProcessed.empty()) {
            save();
        }

    } catch (const fs::filesystem_error& e) {
        result.errorMessage = std::string("Filesystem error: ") + e.what();
    } catch (const std::exception& e) {
        result.errorMessage = std::string("Error: ") + e.what();
    }

    return result;
}

std::vector<std::string> ConfigManager::listArchiveContents(
    const std::string& archivePath) const
{
    Acquire lock(&m_cs);

    std::vector<std::string> contents;

    try {
        std::ifstream archive(archivePath, std::ios::binary);
        if (!archive.is_open()) {
            return contents;
        }

        uint32_t fileCount = 0;
        if (!readArchiveHeader(archive, fileCount)) {
            return contents;
        }

        for (uint32_t i = 0; i < fileCount; ++i) {
            std::string relativePath;
            std::vector<char> content;
            if (!readArchiveEntry(archive, relativePath, content)) {
                break;
            }
            contents.push_back(relativePath);
        }

    } catch (const std::exception&) {
        // Ignore errors, return what we have
    }

    return contents;
}

bool ConfigManager::validateArchive(const std::string& archivePath) const
{
    Acquire lock(&m_cs);

    try {
        std::ifstream archive(archivePath, std::ios::binary);
        if (!archive.is_open()) {
            return false;
        }

        uint32_t fileCount = 0;
        if (!readArchiveHeader(archive, fileCount)) {
            return false;
        }

        // Validate we can read all entries
        for (uint32_t i = 0; i < fileCount; ++i) {
            std::string relativePath;
            std::vector<char> content;
            if (!readArchiveEntry(archive, relativePath, content)) {
                return false;
            }

            // Check for path traversal attacks
            if (relativePath.find("..") != std::string::npos ||
                relativePath[0] == '/' || relativePath[0] == '\\') {
                return false;
            }
        }

        return true;

    } catch (const std::exception&) {
        return false;
    }
}

// ==================== Templates Implementation ====================

// Embedded template contents
// These are the actual template file contents embedded in the binary
// so templates work even without external template files

static const char* const TEMPLATE_DEFAULT = R"TEMPLATE(#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# yamy - default.mayu
# Default configuration template with common keyboard remappings
#
# This template provides essential keyboard customizations that most users
# find helpful. It serves as a good starting point for further customization.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Keyboard Type Detection
# Automatically detects whether you have a 104-key (US) or 109-key (JP) layout
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

if ( !KBD109 ) and ( !KBD104 )
  # Default to 109-key Japanese keyboard layout if not specified
  # Change this if you have a different keyboard layout
endif


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# CapsLock to Control
# One of the most popular remappings - makes CapsLock act as Control
# This reduces strain on your pinky and makes Ctrl combinations easier
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap Global

# Remap CapsLock to Left Control
mod control += CapsLock
key *CapsLock = *LControl

# Also handle E0-prefixed CapsLock (some keyboards send this)
mod control += E0CapsLock
key *E0CapsLock = *LControl


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Window Management Shortcuts
# Useful keyboard shortcuts for managing windows
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Move window with Ctrl+Shift+Arrow keys
key C-S-Left   = &WindowMove(-16, 0)   # Move window left
key C-S-Right  = &WindowMove(16, 0)    # Move window right
key C-S-Up     = &WindowMove(0, -16)   # Move window up
key C-S-Down   = &WindowMove(0, 16)    # Move window down

# Fine-grained window movement with Ctrl+Shift+Alt+Arrow
key C-S-A-Left  = &WindowMove(-1, 0)   # Move window left (1 pixel)
key C-S-A-Right = &WindowMove(1, 0)    # Move window right (1 pixel)
key C-S-A-Up    = &WindowMove(0, -1)   # Move window up (1 pixel)
key C-S-A-Down  = &WindowMove(0, 1)    # Move window down (1 pixel)

# Window state shortcuts
key C-S-Z = &WindowMaximize     # Maximize window
key C-S-I = &WindowMinimize     # Minimize window
key C-S-X = &WindowVMaximize    # Maximize window vertically
key C-S-C = &WindowHMaximize    # Maximize window horizontally


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Mouse Movement via Keyboard
# Use Win+Arrow keys to move the mouse cursor
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

key W-Left  = &MouseMove(-16, 0)   # Move mouse left
key W-Right = &MouseMove(16, 0)    # Move mouse right
key W-Up    = &MouseMove(0, -16)   # Move mouse up
key W-Down  = &MouseMove(0, 16)    # Move mouse down


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Utility Shortcuts
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Reload configuration (Ctrl+Shift+S)
key C-S-S = &LoadSetting

# Show window information (Ctrl+Shift+D) - useful for debugging
key C-S-D = &WindowIdentify


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Dialog Box Handling
# Make Escape and Ctrl+G close dialog boxes
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

window DialogBox /:#32770:/ : Global
  key C-G = Escape
)TEMPLATE";

static const char* const TEMPLATE_EMACS = R"TEMPLATE(#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# yamy - emacs.mayu
# Emacs-style keybindings template
#
# This template provides Emacs-like keybindings across all applications.
# If you're familiar with Emacs, this will make other apps feel more natural.
#
# Key conventions:
#   C- = Control
#   M- = Alt (Meta)
#   S- = Shift
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# CapsLock to Control
# Essential for comfortable Emacs usage - CapsLock becomes Control
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap Global

mod control += CapsLock
key *CapsLock = *LControl
mod control += E0CapsLock
key *E0CapsLock = *LControl


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Emacs Movement Commands
# These work in text fields across most applications
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap EmacsMove : Global

# Basic cursor movement
key C-F = Right               # Forward one character
key C-B = Left                # Backward one character
key C-N = Down                # Next line
key C-P = Up                  # Previous line
key C-A = Home                # Beginning of line
key C-E = End                 # End of line

# Word movement (Alt+arrow equivalent)
key M-F = C-Right             # Forward one word
key M-B = C-Left              # Backward one word

# Page movement
key C-V = Next                # Scroll down (Page Down)
key M-V = Prior               # Scroll up (Page Up)

# Document navigation
key Home = C-Home             # Beginning of document
key End = C-End               # End of document
key S-M-Comma = C-Home        # M-< (Beginning of buffer)
key S-M-Period = C-End        # M-> (End of buffer)

# Scrolling without moving cursor
key C-L = &WindowRedraw       # Recenter/redraw

# Cancel command
key C-G = Escape              # Cancel current operation

# Search
key C-S = C-F                 # Incremental search forward


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Emacs Editing Commands
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap EmacsEdit : EmacsMove

# Deletion
key C-D = Delete              # Delete character forward
key C-H = BackSpace           # Delete character backward (backspace)
key M-D = S-C-Right C-X       # Kill word forward
key M-BackSpace = S-C-Left C-X  # Kill word backward

# Line operations
key C-K = S-End C-X           # Kill to end of line

# Character transpose
key C-T = S-Right C-X Left C-V Right  # Transpose characters

# Enter/newline
key C-J = Return              # Newline
key C-M = Return              # Carriage return (same as Enter)
key C-O = Return Left         # Open line (insert newline, stay in place)

# Cut, Copy, Paste (Emacs style)
key C-W = C-X                 # Kill region (Cut)
key M-W = C-C                 # Copy region
key C-Y = C-V                 # Yank (Paste)

# Undo
key C-Slash = C-Z             # Undo
key C-Underscore = C-Z        # Undo (alternative)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# C-x Prefix Commands
# Emacs uses C-x as a prefix for many commands
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap2 EmacsC-X : EmacsEdit
  event prefixed = &HelpMessage("C-x", "C-x prefix active")
  event before-key-down = &HelpMessage

  key C-S = C-S               # Save file
  key C-W = LAlt F A          # Save As (Write file)
  key C-F = C-O               # Open file (Find file)
  key K = C-N                 # New file (Kill buffer, then new)
  key C-C = A-F4              # Exit application
  key U = C-Z                 # Undo

keymap EmacsEdit
  key C-X = &Prefix(EmacsC-X)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Mark and Selection
# C-Space sets the mark for text selection
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap2 EmacsMark : EmacsEdit
  # Movement with selection (extends selection)
  key C-F = S-Right &Prefix(EmacsMark)
  key C-B = S-Left &Prefix(EmacsMark)
  key C-N = S-Down &Prefix(EmacsMark)
  key C-P = S-Up &Prefix(EmacsMark)
  key C-A = S-Home &Prefix(EmacsMark)
  key C-E = S-End &Prefix(EmacsMark)
  key M-F = S-C-Right &Prefix(EmacsMark)
  key M-B = S-C-Left &Prefix(EmacsMark)
  key C-V = S-Next &Prefix(EmacsMark)
  key M-V = S-Prior &Prefix(EmacsMark)
  key Home = S-C-Home &Prefix(EmacsMark)
  key End = S-C-End &Prefix(EmacsMark)

  # Arrow keys with selection
  key Left = S-Left &Prefix(EmacsMark)
  key Right = S-Right &Prefix(EmacsMark)
  key Up = S-Up &Prefix(EmacsMark)
  key Down = S-Down &Prefix(EmacsMark)

  # Cut and copy end mark mode
  key C-W = C-X Left Right    # Kill region
  key M-W = C-C Left Right    # Copy region

  # Cancel mark
  key C-G = Left Right &Undefined

keymap EmacsEdit
  key C-Space = &Prefix(EmacsMark)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Application-Specific Settings
# Apply EmacsEdit keymap to text input controls
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Standard Windows edit controls
window EditControl /:(Edit|TEdit|RichEdit(20[AW])?)$/ : EmacsEdit

# Combo boxes (dropdown with text input)
window ComboBox /:ComboBox(:Edit)?$/ : EmacsEdit

# List views (for navigation)
window SysListView32 /:SysListView32$/ : EmacsMove

# Tree views (for navigation)
window SysTreeView32 /:SysTreeView32$/ : EmacsMove


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Dialog Box Handling
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

window DialogBox /:#32770:/ : Global
  key C-G = Escape            # Cancel dialog with C-g


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Exclude Real Emacs
# Don't apply these remappings in actual Emacs
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap Emacsen : Global
  # Pass through all special keys in real Emacs

window Meadow /:Meadow$/ : Emacsen
window Emacs /:Emacs$/ : Emacsen


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Utility Shortcuts
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap Global
  # Reload configuration
  key C-S-S = &LoadSetting
)TEMPLATE";

static const char* const TEMPLATE_VIM = R"TEMPLATE(#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# yamy - vim.mayu
# Vim-style keybindings template
#
# This template provides Vim-like keybindings for navigation and editing
# across applications. Useful for Vim users who want consistent keybindings.
#
# Note: This provides basic Vim motions, not full Vim emulation.
# For complete Vim behavior, consider a dedicated Vim emulator.
#
# Escape is used to enter "normal mode" where h/j/k/l become movement keys.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# CapsLock to Escape
# Many Vim users prefer CapsLock as Escape for faster mode switching
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap Global

# Remap CapsLock to Escape (common Vim user preference)
key *CapsLock = *Escape
key *E0CapsLock = *Escape


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Vim Normal Mode
# Press Escape to enter this mode where h/j/k/l become movement keys
# Press i, a, or other insert commands to return to insert mode
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap2 VimNormal : Global
  event prefixed = &HelpMessage("VIM", "-- NORMAL --")
  event before-key-down = &HelpMessage

  # Basic movement (h/j/k/l)
  key H = Left                # Move left
  key J = Down                # Move down
  key K = Up                  # Move up
  key L = Right               # Move right

  # Word movement
  key W = C-Right             # Forward to start of next word
  key B = C-Left              # Backward to start of word
  key E = C-Right Left        # Forward to end of word

  # Line movement
  key _0 = Home               # Beginning of line
  key S-_4 = End              # End of line ($)
  key S-_6 = Home             # First non-blank character (^)

  # Document movement
  key G G = C-Home            # Go to start of document
  key S-G = C-End             # Go to end of document

  # Page movement
  key C-F = Next              # Page forward (Page Down)
  key C-B = Prior             # Page backward (Page Up)
  key C-D = Next              # Half page down (simplified)
  key C-U = Prior             # Half page up (simplified)

  # Insert mode transitions
  key I = &Undefined          # Insert before cursor (exit normal mode)
  key A = Right &Undefined    # Append after cursor
  key S-I = Home &Undefined   # Insert at beginning of line
  key S-A = End &Undefined    # Append at end of line
  key O = End Return &Undefined  # Open line below
  key S-O = Home Return Up &Undefined  # Open line above

  # Editing in normal mode
  key X = Delete              # Delete character under cursor
  key S-X = BackSpace         # Delete character before cursor
  key R = &Prefix(VimReplace) # Replace single character

  # Delete operations
  key D D = Home S-End C-X    # Delete entire line
  key D W = S-C-Right C-X     # Delete word
  key D S-_4 = S-End C-X      # Delete to end of line (d$)
  key S-D = S-End C-X         # Delete to end of line (D)

  # Yank (copy) operations
  key Y Y = Home S-End C-C Right  # Yank entire line
  key Y W = S-C-Right C-C Left    # Yank word
  key Y S-_4 = S-End C-C      # Yank to end of line

  # Put (paste)
  key P = C-V                 # Put after cursor
  key S-P = C-V Left          # Put before cursor

  # Change operations (delete and enter insert mode)
  key C C = Home S-End C-X &Undefined  # Change entire line
  key C W = S-C-Right C-X &Undefined   # Change word
  key S-C = S-End C-X &Undefined       # Change to end of line

  # Undo/Redo
  key U = C-Z                 # Undo
  key C-R = C-Y               # Redo

  # Search
  key Slash = C-F             # Search forward
  key N = F3                  # Next search result
  key S-N = S-F3              # Previous search result

  # Join lines
  key S-J = End Delete Space  # Join lines

  # Cancel/Escape stays in normal mode
  key Escape = &Prefix(VimNormal)

keymap Global
  # Enter normal mode with Escape
  key Escape = &Prefix(VimNormal)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Vim Replace Mode
# Replace a single character, then return to normal mode
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap2 VimReplace : VimNormal
  event prefixed = &HelpMessage("VIM", "-- REPLACE --")
  event before-key-down = &HelpMessage


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Vim Visual Mode (Selection)
# Press v in normal mode to start visual selection
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap2 VimVisual : Global
  event prefixed = &HelpMessage("VIM", "-- VISUAL --")
  event before-key-down = &HelpMessage

  # Movement with selection
  key H = S-Left &Prefix(VimVisual)
  key J = S-Down &Prefix(VimVisual)
  key K = S-Up &Prefix(VimVisual)
  key L = S-Right &Prefix(VimVisual)
  key W = S-C-Right &Prefix(VimVisual)
  key B = S-C-Left &Prefix(VimVisual)
  key _0 = S-Home &Prefix(VimVisual)
  key S-_4 = S-End &Prefix(VimVisual)

  # Visual mode operations
  key Y = C-C &Prefix(VimNormal)  # Yank selection
  key D = C-X &Prefix(VimNormal)  # Delete selection
  key X = C-X &Prefix(VimNormal)  # Delete selection
  key C = C-X &Undefined          # Change selection (delete and insert)

  # Exit visual mode
  key Escape = Right &Prefix(VimNormal)
  key V = Right &Prefix(VimNormal)

keymap2 VimNormal
  # Enter visual mode
  key V = &Prefix(VimVisual)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Command Line Mode (simplified)
# : commands for common operations
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap2 VimCommand : Global
  event prefixed = &HelpMessage("VIM", ":")
  event before-key-down = &HelpMessage

  key W Return = C-S &Prefix(VimNormal)   # :w - save
  key Q Return = A-F4                      # :q - quit
  key Q S-_1 Return = A-F4                # :q! - force quit
  key W Q Return = C-S A-F4               # :wq - save and quit
  key X Return = C-S A-F4                 # :x - save and quit

  # Cancel command
  key Escape = &Prefix(VimNormal)

keymap2 VimNormal
  # Enter command mode
  key S-Semicolon = &Prefix(VimCommand)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Application-Specific Settings
# Only enable Vim keys in text editing contexts
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

window EditControl /:(Edit|TEdit|RichEdit(20[AW])?)$/ : Global
  key Escape = &Prefix(VimNormal)

window ComboBox /:ComboBox(:Edit)?$/ : Global
  key Escape = &Prefix(VimNormal)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Exclude Real Vim/Terminal Applications
# Don't apply these remappings in actual Vim or terminal emulators
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap VimExclude : Global
  # Pass through all keys - don't remap in real Vim

window GVim /gvim.*:Vim$/ : VimExclude
window Vim /vim:/ : VimExclude
window Terminal /:(ConsoleWindowClass|mintty|Terminal)$/ : VimExclude


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Global Shortcuts (always available)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

keymap Global
  # Reload configuration
  key C-S-S = &LoadSetting

  # Window management (Vim-inspired)
  key C-W H = &WindowMove(-16, 0)   # Move window left
  key C-W J = &WindowMove(0, 16)    # Move window down
  key C-W K = &WindowMove(0, -16)   # Move window up
  key C-W L = &WindowMove(16, 0)    # Move window right
)TEMPLATE";

std::string ConfigManager::getTemplatesDir()
{
    // First try the installed location
    std::string configDir = getDefaultConfigDir();
    if (!configDir.empty()) {
        std::string templatesDir = configDir + "/templates";
        if (fs::exists(templatesDir)) {
            return templatesDir;
        }
    }

    // Try relative to executable (for development)
    try {
        fs::path exePath = fs::current_path();
        fs::path templatesPath = exePath / "src" / "resources" / "templates";
        if (fs::exists(templatesPath)) {
            return templatesPath.string();
        }
    } catch (const fs::filesystem_error&) {
        // Ignore
    }

    return "";
}

std::vector<std::string> ConfigManager::listTemplates()
{
    return {"default", "emacs", "vim"};
}

ConfigManager::TemplateResult ConfigManager::createFromTemplate(
    const std::string& templateNameStr,
    const std::string& targetPath)
{
    // Map string to enum
    std::string lowerName = templateNameStr;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    if (lowerName == "default") {
        return createFromTemplate(TemplateName::Default, targetPath);
    } else if (lowerName == "emacs") {
        return createFromTemplate(TemplateName::Emacs, targetPath);
    } else if (lowerName == "vim") {
        return createFromTemplate(TemplateName::Vim, targetPath);
    }

    TemplateResult result;
    result.errorMessage = "Unknown template: " + templateNameStr +
                         ". Available: default, emacs, vim";
    return result;
}

ConfigManager::TemplateResult ConfigManager::createFromTemplate(
    TemplateName templateName,
    const std::string& targetPath)
{
    Acquire lock(&m_cs);

    TemplateResult result;

    // Check if target already exists
    if (fileExists(targetPath)) {
        result.errorMessage = "Target file already exists: " + targetPath;
        return result;
    }

    // Get template content
    const char* templateContent = nullptr;
    std::string templateDisplayName;

    switch (templateName) {
        case TemplateName::Default:
            templateContent = TEMPLATE_DEFAULT;
            templateDisplayName = "Default";
            break;
        case TemplateName::Emacs:
            templateContent = TEMPLATE_EMACS;
            templateDisplayName = "Emacs";
            break;
        case TemplateName::Vim:
            templateContent = TEMPLATE_VIM;
            templateDisplayName = "Vim";
            break;
        default:
            result.errorMessage = "Invalid template name";
            return result;
    }

    try {
        // Create parent directory if needed
        fs::path targetFile(targetPath);
        fs::path parentDir = targetFile.parent_path();
        if (!parentDir.empty() && !fs::exists(parentDir)) {
            fs::create_directories(parentDir);
        }

        // Write template to target file
        std::ofstream outFile(targetPath);
        if (!outFile.is_open()) {
            result.errorMessage = "Cannot create file: " + targetPath;
            return result;
        }

        outFile << templateContent;
        outFile.close();

        if (!outFile.good()) {
            result.errorMessage = "Error writing to file: " + targetPath;
            return result;
        }

        // Add to config manager's list
        int index = findConfig(targetPath);
        if (index < 0) {
            std::string name = extractName(targetPath);
            m_configs.emplace_back(targetPath, name, true);
            save();
        }

        // Create metadata for the new config
        ConfigMetadata metadata;
        metadata.setName(extractName(targetPath));
        metadata.setDescription("Created from " + templateDisplayName + " template");
        metadata.save(targetPath);

        result.success = true;
        result.createdPath = targetPath;

    } catch (const fs::filesystem_error& e) {
        result.errorMessage = std::string("Filesystem error: ") + e.what();
    } catch (const std::exception& e) {
        result.errorMessage = std::string("Error: ") + e.what();
    }

    return result;
}
