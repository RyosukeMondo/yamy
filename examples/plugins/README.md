# Yamy Example Plugin

This directory contains an example plugin demonstrating the Yamy plugin interface.

## Plugin Overview

The `ExamplePlugin` demonstrates:

- Implementing the `IPlugin` interface
- Registering callbacks with `NotificationDispatcher`
- Handling configuration and engine lifecycle events
- Exporting the required `plugin_create` factory function
- Proper cleanup in `shutdown()`

## Building the Plugin

### Prerequisites

- CMake 3.16+
- C++17 compatible compiler (GCC 8+, Clang 7+)
- Yamy source tree (headers required)

### Build from within Yamy source tree

```bash
cd examples/plugins
mkdir build && cd build
cmake ..
make
```

### Build as part of main Yamy build

Add the following to the main CMakeLists.txt:

```cmake
add_subdirectory(examples/plugins)
```

## Installation

### User installation (recommended)

```bash
cd build
make install-user
```

This installs to `~/.local/share/yamy/plugins/`.

### System installation

```bash
cd build
sudo make install
```

This installs to the prefix specified at cmake time.

### Manual installation

```bash
cp build/example_plugin.so ~/.local/share/yamy/plugins/
```

## Plugin Interface

All plugins must implement the `yamy::core::IPlugin` interface:

```cpp
class IPlugin {
public:
    virtual const char* getName() const = 0;      // Plugin display name
    virtual const char* getVersion() const = 0;   // Version string
    virtual int getApiVersion() const = 0;        // API compatibility version
    virtual bool initialize(Engine* engine) = 0;  // Setup, register callbacks
    virtual void shutdown() = 0;                  // Cleanup, unregister callbacks
};
```

### Required Export

Plugins must export a C-linkage factory function:

```cpp
extern "C" yamy::core::IPlugin* plugin_create();
```

### Optional Export

Plugins may optionally export a destroy function:

```cpp
extern "C" void plugin_destroy(yamy::core::IPlugin* plugin);
```

If not provided, `delete` is used.

## Notification Events

Plugins can subscribe to these notification types via `NotificationDispatcher`:

### Configuration Events
- `ConfigLoading` - Configuration file is being loaded
- `ConfigLoaded` - Configuration loaded successfully
- `ConfigError` - Configuration error occurred (data contains message)

### Engine Events
- `EngineStarting` - Engine is starting up
- `EngineStarted` - Engine has started
- `EngineStopping` - Engine is shutting down
- `EngineStopped` - Engine has stopped
- `EngineError` - Engine error occurred

### Runtime Events
- `KeymapSwitched` - Active keymap changed
- `FocusChanged` - Window focus changed
- `ModifierChanged` - Modifier key state changed

### Performance Events
- `LatencyReport` - Key processing latency data
- `CpuUsageReport` - CPU usage statistics

## Example: Registering a Callback

```cpp
#include "core/notification_dispatcher.h"
#include "core/platform/ipc_defs.h"

bool MyPlugin::initialize(Engine* engine) {
    // Subscribe to specific events
    std::unordered_set<yamy::MessageType> events = {
        yamy::MessageType::ConfigLoaded,
        yamy::MessageType::KeymapSwitched
    };

    m_callbackHandle = yamy::core::NotificationDispatcher::instance()
        .registerCallback(events, [this](yamy::MessageType type, const std::string& data) {
            // Handle the event
            if (type == yamy::MessageType::ConfigLoaded) {
                onConfigLoaded();
            }
        });

    return true;
}

void MyPlugin::shutdown() {
    // Always unregister to avoid dangling references
    yamy::core::NotificationDispatcher::instance()
        .unregisterCallback(m_callbackHandle);
}
```

## Best Practices

1. **Always unregister callbacks in shutdown()** - Failing to do so causes crashes

2. **Check API version** - Return `PLUGIN_API_VERSION` from `getApiVersion()`

3. **Handle null engine** - The engine pointer may be nullptr during early initialization

4. **Catch exceptions in callbacks** - Don't let exceptions propagate

5. **Log important events** - Use std::cout/cerr for debugging

6. **Keep plugins simple** - Complex logic belongs in the main application

## Troubleshooting

### Plugin not loading

1. Check the plugin is in `~/.local/share/yamy/plugins/`
2. Verify file has `.so` extension
3. Check file permissions (`chmod +x`)
4. Run `ldd plugin.so` to check dependencies

### Symbols not found

Ensure you export with C linkage:
```cpp
extern "C" yamy::core::IPlugin* plugin_create();
```

### Callbacks not firing

1. Verify registration succeeded (check handle != 0)
2. Ensure the event type is in your subscribed set
3. Check that notifications are being dispatched
