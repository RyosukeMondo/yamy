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
#include <fstream>
#include <cstring>

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
