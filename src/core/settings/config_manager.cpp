// config_manager.cpp
// Configuration manager implementation

#include "config_manager.h"
#include <filesystem>
#include <algorithm>
#include <cstdlib>

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
