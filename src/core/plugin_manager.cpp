#include "plugin_manager.h"
#include "utils/platform_logger.h"

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
        PLATFORM_LOG_WARN("plugin", "PluginManager already initialized");
        return true;
    }

    m_engine = engine;
    m_initialized = true;

    PLATFORM_LOG_INFO("plugin", "PluginManager initialized, scanning for plugins...");
    scanAndLoadPlugins();

    return true;
}

void PluginManager::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        return;
    }

    PLATFORM_LOG_INFO("plugin", "PluginManager shutting down, unloading %zu plugins",
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
        PLATFORM_LOG_WARN("plugin", "Could not determine plugin directory");
        return;
    }

    // Check if directory exists
    struct stat st;
    if (stat(pluginDir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        PLATFORM_LOG_INFO("plugin", "Plugin directory does not exist: %s", pluginDir.c_str());
        return;
    }

    DIR* dir = opendir(pluginDir.c_str());
    if (!dir) {
        PLATFORM_LOG_WARN("plugin", "Could not open plugin directory: %s", pluginDir.c_str());
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
        PLATFORM_LOG_INFO("plugin", "Found plugin file: %s", fullPath.c_str());

        // Load the plugin
        void* handle = dlopen(fullPath.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            PLATFORM_LOG_ERROR("plugin", "Failed to load plugin %s: %s",
                              fullPath.c_str(), dlerror());
            continue;
        }

        // Clear any existing error
        dlerror();

        // Look for plugin_create factory function
        auto createFunc = reinterpret_cast<PluginCreateFunc>(
            dlsym(handle, "plugin_create"));

        const char* dlsymError = dlerror();
        if (dlsymError || !createFunc) {
            PLATFORM_LOG_ERROR("plugin", "Plugin %s missing plugin_create function: %s",
                              fullPath.c_str(), dlsymError ? dlsymError : "symbol not found");
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
            PLATFORM_LOG_ERROR("plugin", "Plugin %s factory threw exception: %s",
                              fullPath.c_str(), e.what());
            dlclose(handle);
            continue;
        } catch (...) {
            PLATFORM_LOG_ERROR("plugin", "Plugin %s factory threw unknown exception",
                              fullPath.c_str());
            dlclose(handle);
            continue;
        }

        if (!plugin) {
            PLATFORM_LOG_ERROR("plugin", "Plugin %s factory returned nullptr", fullPath.c_str());
            dlclose(handle);
            continue;
        }

        // Check API version
        int pluginApiVersion = plugin->getApiVersion();
        if (pluginApiVersion != PLUGIN_API_VERSION) {
            PLATFORM_LOG_ERROR("plugin", "Plugin %s API version mismatch: expected %d, got %d",
                              fullPath.c_str(), PLUGIN_API_VERSION, pluginApiVersion);
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
            PLATFORM_LOG_ERROR("plugin", "Plugin %s returned empty name", fullPath.c_str());
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
                PLATFORM_LOG_WARN("plugin", "Plugin %s already loaded, skipping duplicate from %s",
                                 name, fullPath.c_str());
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
            PLATFORM_LOG_ERROR("plugin", "Plugin %s initialization threw exception: %s",
                              name, e.what());
        } catch (...) {
            PLATFORM_LOG_ERROR("plugin", "Plugin %s initialization threw unknown exception", name);
        }

        if (!initSuccess) {
            PLATFORM_LOG_ERROR("plugin", "Plugin %s failed to initialize", name);
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

        PLATFORM_LOG_INFO("plugin", "Loaded plugin: %s v%s", name, version ? version : "unknown");
    }

    closedir(dir);
    PLATFORM_LOG_INFO("plugin", "Plugin scan complete, %zu plugins loaded", m_plugins.size());
}

bool PluginManager::loadPlugin(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        PLATFORM_LOG_ERROR("plugin", "PluginManager not initialized");
        return false;
    }

    // Check if already loaded from this path
    for (const auto& lp : m_plugins) {
        if (lp.path == path) {
            PLATFORM_LOG_WARN("plugin", "Plugin already loaded from: %s", path.c_str());
            return false;
        }
    }

    // Load the plugin
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        PLATFORM_LOG_ERROR("plugin", "Failed to load plugin %s: %s", path.c_str(), dlerror());
        return false;
    }

    dlerror(); // Clear error

    auto createFunc = reinterpret_cast<PluginCreateFunc>(
        dlsym(handle, "plugin_create"));

    const char* dlsymError = dlerror();
    if (dlsymError || !createFunc) {
        PLATFORM_LOG_ERROR("plugin", "Plugin %s missing plugin_create: %s",
                          path.c_str(), dlsymError ? dlsymError : "symbol not found");
        dlclose(handle);
        return false;
    }

    auto destroyFunc = reinterpret_cast<PluginDestroyFunc>(
        dlsym(handle, "plugin_destroy"));

    IPlugin* plugin = nullptr;
    try {
        plugin = createFunc();
    } catch (...) {
        PLATFORM_LOG_ERROR("plugin", "Plugin %s factory threw exception", path.c_str());
        dlclose(handle);
        return false;
    }

    if (!plugin) {
        PLATFORM_LOG_ERROR("plugin", "Plugin %s factory returned nullptr", path.c_str());
        dlclose(handle);
        return false;
    }

    if (plugin->getApiVersion() != PLUGIN_API_VERSION) {
        PLATFORM_LOG_ERROR("plugin", "Plugin %s API version mismatch", path.c_str());
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
        PLATFORM_LOG_ERROR("plugin", "Plugin %s returned empty name", path.c_str());
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
            PLATFORM_LOG_WARN("plugin", "Plugin %s already loaded", name);
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
        PLATFORM_LOG_ERROR("plugin", "Plugin %s initialization threw exception", name);
    }

    if (!initSuccess) {
        PLATFORM_LOG_ERROR("plugin", "Plugin %s failed to initialize", name);
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

    PLATFORM_LOG_INFO("plugin", "Loaded plugin: %s v%s", name,
                      plugin->getVersion() ? plugin->getVersion() : "unknown");
    return true;
}

bool PluginManager::unloadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(m_plugins.begin(), m_plugins.end(),
        [&name](const LoadedPlugin& lp) { return lp.name == name; });

    if (it == m_plugins.end()) {
        PLATFORM_LOG_WARN("plugin", "Plugin not found: %s", name.c_str());
        return false;
    }

    unloadPluginInternal(*it);
    m_plugins.erase(it);

    PLATFORM_LOG_INFO("plugin", "Unloaded plugin: %s", name.c_str());
    return true;
}

void PluginManager::unloadPluginInternal(LoadedPlugin& lp) {
    // Call shutdown on the plugin
    if (lp.plugin) {
        try {
            lp.plugin->shutdown();
        } catch (const std::exception& e) {
            PLATFORM_LOG_ERROR("plugin", "Plugin %s shutdown threw exception: %s",
                              lp.name.c_str(), e.what());
        } catch (...) {
            PLATFORM_LOG_ERROR("plugin", "Plugin %s shutdown threw unknown exception",
                              lp.name.c_str());
        }

        // Destroy the plugin instance
        if (lp.destroyFunc) {
            try {
                lp.destroyFunc(lp.plugin);
            } catch (...) {
                PLATFORM_LOG_ERROR("plugin", "Plugin %s destroy threw exception", lp.name.c_str());
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
