/**
 * @file example_plugin.cpp
 * @brief Example plugin demonstrating the Yamy plugin interface
 *
 * This example shows how to:
 * - Implement the IPlugin interface
 * - Register notification callbacks with NotificationDispatcher
 * - Handle engine lifecycle events
 * - Export the required factory function
 *
 * Build this plugin as a shared library (.so) and place it in:
 *   ~/.local/share/yamy/plugins/
 */

#include "core/plugin_manager.h"
#include "core/notification_dispatcher.h"
#include "core/platform/ipc_defs.h"

#include <iostream>
#include <unordered_set>

namespace {

/**
 * @class ExamplePlugin
 * @brief Demonstrates plugin development for Yamy
 *
 * This plugin registers for configuration and engine lifecycle events
 * and logs messages when they occur. Use this as a template for
 * developing your own plugins.
 */
class ExamplePlugin : public yamy::core::IPlugin {
public:
    ExamplePlugin() = default;
    ~ExamplePlugin() override = default;

    //--------------------------------------------------------------------------
    // IPlugin interface implementation
    //--------------------------------------------------------------------------

    const char* getName() const override {
        return "Example Plugin";
    }

    const char* getVersion() const override {
        return "1.0.0";
    }

    int getApiVersion() const override {
        return yamy::core::PLUGIN_API_VERSION;
    }

    /**
     * @brief Initialize the plugin
     *
     * Called when the plugin is loaded. This is where you should:
     * - Store the engine reference if needed
     * - Register notification callbacks
     * - Perform any setup work
     *
     * @param engine Pointer to the Engine (may be nullptr during early init)
     * @return true if initialization succeeded
     */
    bool initialize(Engine* engine) override {
        m_engine = engine;

        std::cout << "[ExamplePlugin] Initializing..." << std::endl;

        // Register for configuration events
        // The callback will be invoked when config is loaded or errors occur
        std::unordered_set<yamy::MessageType> configEvents = {
            yamy::MessageType::ConfigLoading,
            yamy::MessageType::ConfigLoaded,
            yamy::MessageType::ConfigError
        };

        m_configCallbackHandle = yamy::core::NotificationDispatcher::instance()
            .registerCallback(configEvents, [this](yamy::MessageType type, const std::string& data) {
                handleConfigEvent(type, data);
            });

        // Register for engine lifecycle events
        std::unordered_set<yamy::MessageType> engineEvents = {
            yamy::MessageType::EngineStarting,
            yamy::MessageType::EngineStarted,
            yamy::MessageType::EngineStopping,
            yamy::MessageType::EngineStopped,
            yamy::MessageType::EngineError
        };

        m_engineCallbackHandle = yamy::core::NotificationDispatcher::instance()
            .registerCallback(engineEvents, [this](yamy::MessageType type, const std::string& data) {
                handleEngineEvent(type, data);
            });

        std::cout << "[ExamplePlugin] Initialized successfully" << std::endl;
        return true;
    }

    /**
     * @brief Shutdown the plugin
     *
     * Called before the plugin is unloaded. This is where you should:
     * - Unregister all callbacks
     * - Release any resources
     * - Clean up state
     */
    void shutdown() override {
        std::cout << "[ExamplePlugin] Shutting down..." << std::endl;

        // Unregister our callbacks to avoid dangling references
        auto& dispatcher = yamy::core::NotificationDispatcher::instance();

        if (m_configCallbackHandle != 0) {
            dispatcher.unregisterCallback(m_configCallbackHandle);
            m_configCallbackHandle = 0;
        }

        if (m_engineCallbackHandle != 0) {
            dispatcher.unregisterCallback(m_engineCallbackHandle);
            m_engineCallbackHandle = 0;
        }

        m_engine = nullptr;

        std::cout << "[ExamplePlugin] Shutdown complete" << std::endl;
    }

private:
    //--------------------------------------------------------------------------
    // Event handlers
    //--------------------------------------------------------------------------

    /**
     * @brief Handle configuration-related events
     */
    void handleConfigEvent(yamy::MessageType type, const std::string& data) {
        switch (type) {
            case yamy::MessageType::ConfigLoading:
                std::cout << "[ExamplePlugin] Config loading: " << data << std::endl;
                break;

            case yamy::MessageType::ConfigLoaded:
                std::cout << "[ExamplePlugin] Config loaded successfully" << std::endl;
                onConfigLoaded();
                break;

            case yamy::MessageType::ConfigError:
                std::cerr << "[ExamplePlugin] Config error: " << data << std::endl;
                break;

            default:
                break;
        }
    }

    /**
     * @brief Handle engine lifecycle events
     */
    void handleEngineEvent(yamy::MessageType type, const std::string& data) {
        switch (type) {
            case yamy::MessageType::EngineStarting:
                std::cout << "[ExamplePlugin] Engine starting..." << std::endl;
                break;

            case yamy::MessageType::EngineStarted:
                std::cout << "[ExamplePlugin] Engine started" << std::endl;
                break;

            case yamy::MessageType::EngineStopping:
                std::cout << "[ExamplePlugin] Engine stopping..." << std::endl;
                break;

            case yamy::MessageType::EngineStopped:
                std::cout << "[ExamplePlugin] Engine stopped" << std::endl;
                break;

            case yamy::MessageType::EngineError:
                std::cerr << "[ExamplePlugin] Engine error: " << data << std::endl;
                break;

            default:
                break;
        }
    }

    /**
     * @brief Called when configuration has been loaded
     *
     * This is an example of how plugins can respond to system events.
     * A real plugin might inspect the configuration and adapt behavior.
     */
    void onConfigLoaded() {
        // Example: plugins could query configuration or modify behavior here
        std::cout << "[ExamplePlugin] Ready to process key events" << std::endl;
    }

    //--------------------------------------------------------------------------
    // Member variables
    //--------------------------------------------------------------------------

    Engine* m_engine = nullptr;
    yamy::core::CallbackHandle m_configCallbackHandle = 0;
    yamy::core::CallbackHandle m_engineCallbackHandle = 0;
};

} // anonymous namespace

//------------------------------------------------------------------------------
// Plugin factory functions (C linkage for dlsym)
//------------------------------------------------------------------------------

extern "C" {

/**
 * @brief Create plugin instance (required)
 *
 * This function MUST be exported by all Yamy plugins.
 * It is called by PluginManager::loadPlugin() to create the plugin instance.
 *
 * @return Pointer to newly created IPlugin instance
 */
yamy::core::IPlugin* plugin_create() {
    return new ExamplePlugin();
}

/**
 * @brief Destroy plugin instance (optional)
 *
 * This function is OPTIONAL. If provided, PluginManager will call it
 * to destroy the plugin instance. If not provided, 'delete' is used.
 *
 * @param plugin Pointer to plugin instance to destroy
 */
void plugin_destroy(yamy::core::IPlugin* plugin) {
    delete plugin;
}

} // extern "C"
