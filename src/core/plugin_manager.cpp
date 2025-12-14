// Windows Stub Logic
#ifndef _WIN32
#include "plugin_manager.h"
#include "utils/logger.h"

#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <algorithm>

namespace yamy {
namespace core {

PluginManager& PluginManager::instance() {
    static PluginManager instance;
    return instance;
}

PluginManager::~PluginManager() {
    shutdown();
}

std::string PluginManager::getPluginDirectory() {
    const char* home = getenv("HOME");
    if (!home) {
        return "";
    }
    return std::string(home) + "/.local/share/yamy/plugins/";
}

bool PluginManager::initialize(Engine* engine) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) {
        LOG_WARN("[plugin] PluginManager already initialized");
        return true;
    }

    m_engine = engine;
    m_initialized = true;

    LOG_INFO("[plugin] PluginManager initialized, scanning for plugins...");
    scanAndLoadPlugins();

    return true;
}

void PluginManager::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        return;
    }

    LOG_INFO("[plugin] PluginManager shutting down, unloading {} plugins",
             m_plugins.size());

    // Unload plugins in reverse order (LIFO)
    for (auto it = m_plugins.rbegin(); it != m_plugins.rend(); ++it) {
        unloadPluginInternal(*it);
    }
    m_plugins.clear();

    m_engine = nullptr;
    m_initialized = false;
}

void PluginManager::scanAndLoadPlugins() {
    std::string pluginDir = getPluginDirectory();
    if (pluginDir.empty()) {
        LOG_WARN("[plugin] Could not determine plugin directory");
        return;
    }

    // Check if directory exists
    struct stat st;
    if (stat(pluginDir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        LOG_INFO("[plugin] Plugin directory does not exist: {}", pluginDir);
        return;
    }

    DIR* dir = opendir(pluginDir.c_str());
    if (!dir) {
        LOG_WARN("[plugin] Could not open plugin directory: {}", pluginDir);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;

        // Only process .so files
        if (filename.length() < 3 ||
            filename.substr(filename.length() - 3) != ".so") {
            continue;
        }

        std::string fullPath = pluginDir + filename;

        // Don't hold lock during load (loadPlugin acquires its own lock)
        // But we're already holding lock from scanAndLoadPlugins caller
        // So we need to call internal load logic directly
        LOG_INFO("[plugin] Found plugin file: {}", fullPath);

        // Load the plugin
        void* handle = dlopen(fullPath.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            LOG_ERROR("[plugin] Failed to load plugin {}: {}", fullPath, dlerror());
            continue;
        }

        // Clear any existing error
        dlerror();

        // Look for plugin_create factory function
        auto createFunc = reinterpret_cast<PluginCreateFunc>(
            dlsym(handle, "plugin_create"));

        const char* dlsymError = dlerror();
        if (dlsymError || !createFunc) {
            LOG_ERROR("[plugin] Plugin {} missing plugin_create function: {}",
                      fullPath, dlsymError ? dlsymError : "symbol not found");
            dlclose(handle);
            continue;
        }

        // Optional: look for plugin_destroy
        auto destroyFunc = reinterpret_cast<PluginDestroyFunc>(
            dlsym(handle, "plugin_destroy"));
        // Ignore error - destroy is optional

        // Create plugin instance
        IPlugin* plugin = nullptr;
        try {
            plugin = createFunc();
        } catch (const std::exception& e) {
            LOG_ERROR("[plugin] Plugin {} factory threw exception: {}", fullPath, e.what());
            dlclose(handle);
            continue;
        } catch (...) {
            LOG_ERROR("[plugin] Plugin {} factory threw unknown exception", fullPath);
            dlclose(handle);
            continue;
        }

        if (!plugin) {
            LOG_ERROR("[plugin] Plugin {} factory returned nullptr", fullPath);
            dlclose(handle);
            continue;
        }

        // Check API version
        int pluginApiVersion = plugin->getApiVersion();
        if (pluginApiVersion != PLUGIN_API_VERSION) {
            LOG_ERROR("[plugin] Plugin {} API version mismatch: expected {}, got {}",
                      fullPath, PLUGIN_API_VERSION, pluginApiVersion);
            if (destroyFunc) {
                destroyFunc(plugin);
            } else {
                delete plugin;
            }
            dlclose(handle);
            continue;
        }

        // Get plugin info
        const char* name = plugin->getName();
        const char* version = plugin->getVersion();

        if (!name || strlen(name) == 0) {
            LOG_ERROR("[plugin] Plugin {} returned empty name", fullPath);
            if (destroyFunc) {
                destroyFunc(plugin);
            } else {
                delete plugin;
            }
            dlclose(handle);
            continue;
        }

        // Check for duplicate plugin names
        bool duplicate = false;
        for (const auto& lp : m_plugins) {
            if (lp.name == name) {
                LOG_WARN("[plugin] Plugin {} already loaded, skipping duplicate from {}",
                         name, fullPath);
                duplicate = true;
                break;
            }
        }

        if (duplicate) {
            if (destroyFunc) {
                destroyFunc(plugin);
            } else {
                delete plugin;
            }
            dlclose(handle);
            continue;
        }

        // Initialize the plugin
        bool initSuccess = false;
        try {
            initSuccess = plugin->initialize(m_engine);
        } catch (const std::exception& e) {
            LOG_ERROR("[plugin] Plugin {} initialization threw exception: {}", name, e.what());
        } catch (...) {
            LOG_ERROR("[plugin] Plugin {} initialization threw unknown exception", name);
        }

        if (!initSuccess) {
            LOG_ERROR("[plugin] Plugin {} failed to initialize", name);
            if (destroyFunc) {
                destroyFunc(plugin);
            } else {
                delete plugin;
            }
            dlclose(handle);
            continue;
        }

        // Store loaded plugin
        LoadedPlugin lp;
        lp.handle = handle;
        lp.plugin = plugin;
        lp.path = fullPath;
        lp.name = name;
        lp.destroyFunc = destroyFunc;
        m_plugins.push_back(lp);

        LOG_INFO("[plugin] Loaded plugin: {} v{}", name, version ? version : "unknown");
    }

    closedir(dir);
    LOG_INFO("[plugin] Plugin scan complete, {} plugins loaded", m_plugins.size());
}

bool PluginManager::loadPlugin(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        LOG_ERROR("[plugin] PluginManager not initialized");
        return false;
    }

    // Check if already loaded from this path
    for (const auto& lp : m_plugins) {
        if (lp.path == path) {
            LOG_WARN("[plugin] Plugin already loaded from: {}", path);
            return false;
        }
    }

    // Load the plugin
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        LOG_ERROR("[plugin] Failed to load plugin {}: {}", path, dlerror());
        return false;
    }

    dlerror(); // Clear error

    auto createFunc = reinterpret_cast<PluginCreateFunc>(
        dlsym(handle, "plugin_create"));

    const char* dlsymError = dlerror();
    if (dlsymError || !createFunc) {
        LOG_ERROR("[plugin] Plugin {} missing plugin_create: {}",
                  path, dlsymError ? dlsymError : "symbol not found");
        dlclose(handle);
        return false;
    }

    auto destroyFunc = reinterpret_cast<PluginDestroyFunc>(
        dlsym(handle, "plugin_destroy"));

    IPlugin* plugin = nullptr;
    try {
        plugin = createFunc();
    } catch (...) {
        LOG_ERROR("[plugin] Plugin {} factory threw exception", path);
        dlclose(handle);
        return false;
    }

    if (!plugin) {
        LOG_ERROR("[plugin] Plugin {} factory returned nullptr", path);
        dlclose(handle);
        return false;
    }

    if (plugin->getApiVersion() != PLUGIN_API_VERSION) {
        LOG_ERROR("[plugin] Plugin {} API version mismatch", path);
        if (destroyFunc) {
            destroyFunc(plugin);
        } else {
            delete plugin;
        }
        dlclose(handle);
        return false;
    }

    const char* name = plugin->getName();
    if (!name || strlen(name) == 0) {
        LOG_ERROR("[plugin] Plugin {} returned empty name", path);
        if (destroyFunc) {
            destroyFunc(plugin);
        } else {
            delete plugin;
        }
        dlclose(handle);
        return false;
    }

    // Check for duplicate names
    for (const auto& lp : m_plugins) {
        if (lp.name == name) {
            LOG_WARN("[plugin] Plugin {} already loaded", name);
            if (destroyFunc) {
                destroyFunc(plugin);
            } else {
                delete plugin;
            }
            dlclose(handle);
            return false;
        }
    }

    bool initSuccess = false;
    try {
        initSuccess = plugin->initialize(m_engine);
    } catch (...) {
        LOG_ERROR("[plugin] Plugin {} initialization threw exception", name);
    }

    if (!initSuccess) {
        LOG_ERROR("[plugin] Plugin {} failed to initialize", name);
        if (destroyFunc) {
            destroyFunc(plugin);
        } else {
            delete plugin;
        }
        dlclose(handle);
        return false;
    }

    LoadedPlugin lp;
    lp.handle = handle;
    lp.plugin = plugin;
    lp.path = path;
    lp.name = name;
    lp.destroyFunc = destroyFunc;
    m_plugins.push_back(lp);

    LOG_INFO("[plugin] Loaded plugin: {} v{}",
             name,
             plugin->getVersion() ? plugin->getVersion() : "unknown");
    return true;
}

bool PluginManager::unloadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(m_plugins.begin(), m_plugins.end(),
        [&name](const LoadedPlugin& lp) { return lp.name == name; });

    if (it == m_plugins.end()) {
        LOG_WARN("[plugin] Plugin not found: {}", name);
        return false;
    }

    unloadPluginInternal(*it);
    m_plugins.erase(it);

    LOG_INFO("[plugin] Unloaded plugin: {}", name);
    return true;
}

void PluginManager::unloadPluginInternal(LoadedPlugin& lp) {
    // Call shutdown on the plugin
    if (lp.plugin) {
        try {
            lp.plugin->shutdown();
        } catch (const std::exception& e) {
            LOG_ERROR("[plugin] Plugin {} shutdown threw exception: {}", lp.name, e.what());
        } catch (...) {
            LOG_ERROR("[plugin] Plugin {} shutdown threw unknown exception", lp.name);
        }

        // Destroy the plugin instance
        if (lp.destroyFunc) {
            try {
                lp.destroyFunc(lp.plugin);
            } catch (...) {
                LOG_ERROR("[plugin] Plugin {} destroy threw exception", lp.name);
            }
        } else {
            delete lp.plugin;
        }
        lp.plugin = nullptr;
    }

    // Close the library
    if (lp.handle) {
        dlclose(lp.handle);
        lp.handle = nullptr;
    }
}

std::vector<std::string> PluginManager::getLoadedPlugins() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> names;
    names.reserve(m_plugins.size());
    for (const auto& lp : m_plugins) {
        names.push_back(lp.name);
    }
    return names;
}

bool PluginManager::isPluginLoaded(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::any_of(m_plugins.begin(), m_plugins.end(),
        [&name](const LoadedPlugin& lp) { return lp.name == name; });
}

} // namespace core
} // namespace yamy
#endif

#ifdef _WIN32
#include "plugin_manager.h"
#include "utils/logger.h"

namespace yamy {
namespace core {

PluginManager& PluginManager::instance() {
    static PluginManager instance;
    return instance;
}

PluginManager::~PluginManager() {}
std::string PluginManager::getPluginDirectory() { return ""; }
bool PluginManager::initialize(Engine* engine) { return true; }
void PluginManager::shutdown() {}
void PluginManager::scanAndLoadPlugins() {}
bool PluginManager::loadPlugin(const std::string& path) { return false; }
bool PluginManager::unloadPlugin(const std::string& name) { return false; }
void PluginManager::unloadPluginInternal(LoadedPlugin& lp) {}
std::vector<std::string> PluginManager::getLoadedPlugins() const { return {}; }
bool PluginManager::isPluginLoaded(const std::string& name) const { return false; }

} // namespace core
} // namespace yamy
#endif
