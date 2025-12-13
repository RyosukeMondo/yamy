#pragma once

#ifndef _PLUGIN_MANAGER_H
#define _PLUGIN_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

// Forward declarations
class Engine;

namespace yamy {
namespace core {

/// Plugin API version for compatibility checking
constexpr int PLUGIN_API_VERSION = 1;

/// Plugin interface that all plugins must implement
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /// Get the plugin's display name
    /// @return Human-readable plugin name
    virtual const char* getName() const = 0;

    /// Get the plugin's version string
    /// @return Version string (e.g., "1.0.0")
    virtual const char* getVersion() const = 0;

    /// Get the plugin API version this plugin was built against
    /// @return API version number
    virtual int getApiVersion() const = 0;

    /// Initialize the plugin with access to the engine
    /// @param engine Pointer to the Engine instance (may be nullptr during early init)
    /// @return true if initialization succeeded, false otherwise
    virtual bool initialize(Engine* engine) = 0;

    /// Shutdown the plugin and release resources
    /// Called before the plugin is unloaded
    virtual void shutdown() = 0;
};

/// Factory function type that plugins must export
/// Plugins must define: extern "C" IPlugin* plugin_create()
using PluginCreateFunc = IPlugin* (*)();

/// Destructor function type that plugins may optionally export
/// Plugins can define: extern "C" void plugin_destroy(IPlugin*)
using PluginDestroyFunc = void (*)(IPlugin*);

/// Manages loading, initialization, and lifecycle of plugins
/// Thread-safe singleton pattern
class PluginManager {
public:
    /// Get singleton instance
    static PluginManager& instance();

    /// Initialize plugin manager and scan for plugins
    /// @param engine Pointer to Engine for plugin initialization
    /// @return true if initialization succeeded
    bool initialize(Engine* engine);

    /// Shutdown all plugins and unload libraries
    void shutdown();

    /// Load a specific plugin from path
    /// @param path Full path to the .so plugin file
    /// @return true if plugin loaded and initialized successfully
    bool loadPlugin(const std::string& path);

    /// Unload a specific plugin by name
    /// @param name Plugin name to unload
    /// @return true if plugin was found and unloaded
    bool unloadPlugin(const std::string& name);

    /// Get list of loaded plugin names
    /// @return Vector of plugin names
    std::vector<std::string> getLoadedPlugins() const;

    /// Check if a plugin is loaded
    /// @param name Plugin name to check
    /// @return true if plugin is currently loaded
    bool isPluginLoaded(const std::string& name) const;

    /// Get plugin directory path
    /// @return Path to plugins directory
    static std::string getPluginDirectory();

    // Non-copyable
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

private:
    PluginManager() = default;
    ~PluginManager();

    /// Internal structure to track loaded plugins
    struct LoadedPlugin {
        void* handle;           // dlopen handle
        IPlugin* plugin;        // Plugin instance
        std::string path;       // Path to .so file
        std::string name;       // Plugin name
        PluginDestroyFunc destroyFunc;  // Optional destroy function
    };

    /// Scan plugin directory and load all valid plugins
    void scanAndLoadPlugins();

    /// Unload a single plugin (assumes lock is held)
    void unloadPluginInternal(LoadedPlugin& lp);

    mutable std::mutex m_mutex;
    std::vector<LoadedPlugin> m_plugins;
    Engine* m_engine = nullptr;
    bool m_initialized = false;
};

} // namespace core
} // namespace yamy

#endif // _PLUGIN_MANAGER_H
