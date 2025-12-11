/**
 * @file plugin_test.cpp
 * @brief Test for example plugin loading
 */

#include "core/plugin_manager.h"
#include "core/notification_dispatcher.h"
#include "core/platform/ipc_defs.h"

#include <QtTest/QtTest>
#include <dlfcn.h>
#include <iostream>
#include <string>

using namespace yamy::core;

class PluginTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Clear any existing callbacks
        NotificationDispatcher::instance().clearCallbacks();
    }

    void cleanupTestCase() {
        NotificationDispatcher::instance().clearCallbacks();
    }

    void testPluginSymbols() {
        // Find the plugin - try several locations
        // The test runs from the build directory
        std::vector<std::string> searchPaths = {
            "./bin/example_plugin.so",
            "bin/example_plugin.so",
            "./example_plugin.so",
            "../examples/plugins/build/example_plugin.so"
        };

        void* handle = nullptr;
        std::string foundPath;

        for (const auto& path : searchPaths) {
            // Use RTLD_LAZY because plugin depends on host symbols (NotificationDispatcher)
            handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
            if (handle) {
                foundPath = path;
                break;
            }
        }

        if (!handle) {
            qWarning() << "dlopen error:" << dlerror();
            QSKIP("Example plugin not found - skipping test. Build with -DBUILD_EXAMPLES=ON");
        }

        qInfo() << "Loaded plugin from:" << foundPath.c_str();

        // Verify plugin_create exists
        auto createFunc = (PluginCreateFunc)dlsym(handle, "plugin_create");
        QVERIFY2(createFunc != nullptr, "plugin_create symbol not found");

        // Verify plugin_destroy exists (optional but good practice)
        auto destroyFunc = (PluginDestroyFunc)dlsym(handle, "plugin_destroy");
        QVERIFY2(destroyFunc != nullptr, "plugin_destroy symbol not found");

        dlclose(handle);
    }

    void testPluginInterface() {
        std::vector<std::string> searchPaths = {
            "./bin/example_plugin.so",
            "bin/example_plugin.so",
            "./example_plugin.so"
        };

        void* handle = nullptr;
        for (const auto& path : searchPaths) {
            // Use RTLD_LAZY because plugin depends on host symbols
            handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
            if (handle) break;
        }

        if (!handle) {
            qWarning() << "dlopen error:" << dlerror();
            QSKIP("Example plugin not found");
        }

        auto createFunc = (PluginCreateFunc)dlsym(handle, "plugin_create");
        QVERIFY(createFunc != nullptr);

        // Create plugin instance
        IPlugin* plugin = createFunc();
        QVERIFY(plugin != nullptr);

        // Test interface methods
        QCOMPARE(QString(plugin->getName()), QString("Example Plugin"));
        QCOMPARE(QString(plugin->getVersion()), QString("1.0.0"));
        QCOMPARE(plugin->getApiVersion(), PLUGIN_API_VERSION);

        // Test initialization
        size_t callbacksBefore = NotificationDispatcher::instance().callbackCount();
        bool initResult = plugin->initialize(nullptr);
        QVERIFY(initResult);

        // Plugin should have registered callbacks
        size_t callbacksAfter = NotificationDispatcher::instance().callbackCount();
        QVERIFY2(callbacksAfter > callbacksBefore, "Plugin should register callbacks");

        // Test shutdown cleans up callbacks
        plugin->shutdown();
        size_t callbacksAfterShutdown = NotificationDispatcher::instance().callbackCount();
        QCOMPARE(callbacksAfterShutdown, callbacksBefore);

        // Clean up
        auto destroyFunc = (PluginDestroyFunc)dlsym(handle, "plugin_destroy");
        if (destroyFunc) {
            destroyFunc(plugin);
        } else {
            delete plugin;
        }

        dlclose(handle);
    }

    void testPluginNotificationCallback() {
        std::vector<std::string> searchPaths = {
            "./bin/example_plugin.so",
            "bin/example_plugin.so",
            "./example_plugin.so"
        };

        void* handle = nullptr;
        for (const auto& path : searchPaths) {
            // Use RTLD_LAZY because plugin depends on host symbols
            handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
            if (handle) break;
        }

        if (!handle) {
            qWarning() << "dlopen error:" << dlerror();
            QSKIP("Example plugin not found");
        }

        auto createFunc = (PluginCreateFunc)dlsym(handle, "plugin_create");
        IPlugin* plugin = createFunc();
        QVERIFY(plugin->initialize(nullptr));

        // Dispatch a ConfigLoaded event - plugin should receive it
        // We can't easily verify the output but at least ensure no crash
        NotificationDispatcher::instance().dispatch(
            yamy::MessageType::ConfigLoaded, "test_config.mayu");

        // Dispatch an engine event
        NotificationDispatcher::instance().dispatch(
            yamy::MessageType::EngineStarted, "");

        // Clean up
        plugin->shutdown();

        auto destroyFunc = (PluginDestroyFunc)dlsym(handle, "plugin_destroy");
        if (destroyFunc) {
            destroyFunc(plugin);
        } else {
            delete plugin;
        }

        dlclose(handle);
    }
};

QTEST_MAIN(PluginTest)
#include "plugin_test.moc"
