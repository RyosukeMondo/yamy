# Platform Abstraction Architecture

This document describes the platform abstraction layer (PAL) that enables YAMY to run on both Windows and Linux while sharing core business logic.

## Overview

The platform abstraction layer defines a set of interfaces that encapsulate platform-specific operations. Each interface has separate implementations for Windows (using Win32 API) and Linux (using X11/evdev).

```
                    +------------------+
                    |   Core Engine    |
                    |  (platform-agnostic)
                    +--------+---------+
                             |
            +----------------+----------------+
            |                                 |
   +--------v--------+              +--------v--------+
   | Platform Interfaces           | Platform Types   |
   | IWindowSystem                 | WindowHandle     |
   | IInputHook                    | KeyEvent         |
   | IInputInjector                | Rect, Point, Size|
   +--------+--------+              +-----------------+
            |
   +--------+--------+
   |                 |
+--v---+         +---v---+
|Win32 |         | Linux |
+------+         +-------+
```

## Core Interfaces

### 1. IWindowSystem (`src/core/platform/window_system_interface.h`)

Provides window management and query operations.

```cpp
namespace yamy::platform {

class IWindowSystem {
public:
    virtual ~IWindowSystem() = default;

    // Window queries
    virtual WindowHandle getForegroundWindow() = 0;
    virtual std::string getWindowText(WindowHandle hwnd) = 0;
    virtual std::string getClassName(WindowHandle hwnd) = 0;
    virtual bool getWindowRect(WindowHandle hwnd, Rect* rect) = 0;

    // Window manipulation
    virtual bool setForegroundWindow(WindowHandle hwnd) = 0;
    virtual bool moveWindow(WindowHandle hwnd, const Rect& rect) = 0;
    virtual bool showWindow(WindowHandle hwnd, int cmdShow) = 0;
    virtual bool closeWindow(WindowHandle hwnd) = 0;

    // IPC messaging
    virtual bool sendCopyData(WindowHandle sender, WindowHandle target,
                             const CopyData& data, uint32_t flags,
                             uint32_t timeout_ms, uintptr_t* result) = 0;

    // ... additional methods for monitors, clipboard, etc.
};

IWindowSystem* createWindowSystem();

} // namespace yamy::platform
```

**Windows Implementation** (`src/platform/windows/window_system_win32.cpp`):
- Uses Win32 APIs: `GetForegroundWindow()`, `GetWindowTextW()`, `GetClassNameW()`
- Handles UTF-8 to UTF-16 conversion at API boundary
- Uses `SendMessageTimeout()` with `WM_COPYDATA` for IPC

**Linux Implementation** (`src/platform/linux/window_system_linux.cpp`):
- Uses X11 APIs: `XGetInputFocus()`, `XFetchName()`, `XGetClassHint()`
- Reads `_NET_WM_NAME` property for UTF-8 window titles (EWMH)
- Uses Unix domain sockets for IPC messaging

### 2. IInputHook (`src/core/platform/input_hook_interface.h`)

Captures keyboard and mouse events system-wide.

```cpp
namespace yamy::platform {

using KeyCallback = std::function<bool(const KeyEvent&)>;
using MouseCallback = std::function<bool(const MouseEvent&)>;

class IInputHook {
public:
    virtual ~IInputHook() = default;

    virtual bool install(KeyCallback keyCallback, MouseCallback mouseCallback) = 0;
    virtual void uninstall() = 0;
    virtual bool isInstalled() const = 0;
};

IInputHook* createInputHook();

} // namespace yamy::platform
```

**Windows Implementation** (`src/platform/windows/input_hook_win32.cpp`):
- Uses `SetWindowsHookEx()` with `WH_KEYBOARD_LL` and `WH_MOUSE_LL`
- Hook callbacks run on dedicated threads for responsiveness
- Returns `1` to consume events, or calls `CallNextHookEx()` to pass through

**Linux Implementation** (`src/platform/linux/input_hook_linux.cpp`):
- Uses evdev (`/dev/input/event*`) for direct device access
- Reads input events with `read()` in background threads
- Requires `input` group membership or root privileges
- Uses `EVIOCGRAB` ioctl to grab exclusive access when consuming events

### 3. IInputInjector (`src/core/platform/input_injector_interface.h`)

Synthesizes keyboard and mouse events.

```cpp
namespace yamy::platform {

class IInputInjector {
public:
    virtual ~IInputInjector() = default;

    virtual void inject(const KEYBOARD_INPUT_DATA *data,
                       const InjectionContext &ctx,
                       const void *rawData = 0) = 0;
    virtual void keyDown(KeyCode key) = 0;
    virtual void keyUp(KeyCode key) = 0;
    virtual void mouseMove(int32_t dx, int32_t dy) = 0;
    virtual void mouseButton(MouseButton button, bool down) = 0;
    virtual void mouseWheel(int32_t delta) = 0;
};

IInputInjector* createInputInjector(IWindowSystem* windowSystem);

} // namespace yamy::platform
```

**Windows Implementation** (`src/platform/windows/input_injector_win32.cpp`):
- Uses `SendInput()` API with `INPUT` structures
- Sets `KEYEVENTF_SCANCODE` for hardware-level injection
- Handles extended key codes properly

**Linux Implementation** (`src/platform/linux/input_injector_linux.cpp`):
- Uses `uinput` kernel module for virtual device creation
- Creates virtual keyboard/mouse devices in `/dev/uinput`
- Writes `EV_KEY`, `EV_REL`, `EV_SYN` events

## Type Abstractions

### Platform-Agnostic Types (`src/core/platform/types.h`)

```cpp
namespace yamy::platform {

// Window handle (opaque pointer)
using WindowHandle = void*;

// Geometry types
struct Point { int32_t x, y; };
struct Rect { int32_t left, top, right, bottom; };
struct Size { int32_t cx, cy; };

// Input events
struct KeyEvent {
    KeyCode key;
    bool isKeyDown;
    bool isExtended;
    uint32_t scanCode;
    uint32_t timestamp;
    uint32_t flags;
    uintptr_t extraInfo;
};

struct MouseEvent {
    Point pt;
    uint32_t mouseData;
    uint32_t flags;
    uint32_t time;
    uintptr_t extraInfo;
    uint32_t message;
};

// Window show commands
enum class WindowShowCmd {
    Normal,
    Maximized,
    Minimized,
    Unknown
};

} // namespace yamy::platform
```

### Keycode Mapping (`src/platform/linux/keycode_mapping.h`)

Translates between Windows scancodes/VK codes and Linux evdev keycodes:

```cpp
namespace yamy::platform {

uint16_t evdevToYamyKeyCode(uint16_t evdev_code);
uint16_t yamyToEvdevKeyCode(uint16_t yamy_code);
bool isModifierKey(uint16_t evdev_code);

} // namespace yamy::platform
```

## Usage Example

```cpp
#include "core/platform/window_system_interface.h"
#include "core/platform/input_hook_interface.h"
#include "core/platform/input_injector_interface.h"

using namespace yamy::platform;

int main() {
    // Create platform implementations via factory functions
    std::unique_ptr<IWindowSystem> windowSystem(createWindowSystem());
    std::unique_ptr<IInputHook> inputHook(createInputHook());
    std::unique_ptr<IInputInjector> inputInjector(
        createInputInjector(windowSystem.get()));

    // Use platform-agnostic API
    WindowHandle foreground = windowSystem->getForegroundWindow();
    std::string title = windowSystem->getWindowText(foreground);
    std::string className = windowSystem->getClassName(foreground);

    // Install input hook
    inputHook->install(
        [&](const KeyEvent& ev) {
            // Process key event, return true to consume
            if (ev.scanCode == 0x1E && ev.isKeyDown) {  // 'A' key
                inputInjector->keyDown(KeyCode::Unknown);  // Inject 'B'
                inputInjector->keyUp(KeyCode::Unknown);
                return true;  // Consume original
            }
            return false;  // Pass through
        },
        nullptr  // No mouse callback
    );

    // Application event loop...

    inputHook->uninstall();
    return 0;
}
```

## Adding a New Platform

To add support for a new platform (e.g., macOS):

### Step 1: Create Platform Directory

```
src/platform/macos/
  window_system_macos.h
  window_system_macos.cpp
  input_hook_macos.h
  input_hook_macos.cpp
  input_injector_macos.h
  input_injector_macos.cpp
```

### Step 2: Implement Interfaces

Each implementation must:
1. Inherit from the corresponding interface
2. Implement all pure virtual methods
3. Handle platform-specific initialization/cleanup

```cpp
// window_system_macos.h
#pragma once
#include "../../core/platform/window_system_interface.h"

namespace yamy::platform {

class WindowSystemMacOS : public IWindowSystem {
public:
    WindowHandle getForegroundWindow() override;
    std::string getWindowText(WindowHandle hwnd) override;
    // ... implement all methods
};

} // namespace yamy::platform
```

### Step 3: Update Factory Functions

In each implementation file, define the factory when building for that platform:

```cpp
// window_system_macos.cpp
#ifdef __APPLE__
#include "window_system_macos.h"

namespace yamy::platform {

IWindowSystem* createWindowSystem() {
    return new WindowSystemMacOS();
}

} // namespace yamy::platform
#endif
```

### Step 4: Update CMakeLists.txt

```cmake
if(APPLE)
    set(PLATFORM_SOURCES
        src/platform/macos/window_system_macos.cpp
        src/platform/macos/input_hook_macos.cpp
        src/platform/macos/input_injector_macos.cpp
    )
    find_library(COCOA_LIBRARY Cocoa)
    find_library(CARBON_LIBRARY Carbon)
    target_link_libraries(yamy ${COCOA_LIBRARY} ${CARBON_LIBRARY})
endif()
```

## Platform-Specific Considerations

### Windows

- **String Encoding**: Windows uses UTF-16 internally. Convert to UTF-8 at interface boundaries using `WideCharToMultiByte()`/`MultiByteToWideChar()`.
- **Privileges**: Low-level keyboard hooks require the process to have a message pump.
- **UIPI**: Use `ChangeWindowMessageFilter()` to receive messages from elevated processes.

### Linux

- **X11 vs Wayland**: Current implementation requires X11. Wayland support would require different APIs (layer-shell protocol).
- **Permissions**: evdev access requires `input` group membership. uinput requires write access to `/dev/uinput`.
- **Window Managers**: Use EWMH (`_NET_*` atoms) for modern window manager compatibility.
- **Display Connection**: Store and reuse the X11 `Display*` connection, don't open multiple connections.

## Testing

Platform implementations should be unit tested with mock dependencies:

```cpp
// tests/platform/window_system_linux_test.cpp
TEST(WindowSystemLinux, GetForegroundWindow) {
    auto ws = createWindowSystem();
    WindowHandle fg = ws->getForegroundWindow();
    // Verify handle is valid
}

TEST(WindowSystemLinux, GetWindowText) {
    auto ws = createWindowSystem();
    // Create test window, verify text retrieval
}
```

For integration testing without a display server, use Xvfb:

```bash
Xvfb :99 -screen 0 1024x768x24 &
export DISPLAY=:99
./yamy_tests
```

## File Structure

```
src/
  core/
    platform/
      types.h                    # Platform-agnostic types
      window_system_interface.h  # IWindowSystem interface
      input_hook_interface.h     # IInputHook interface
      input_injector_interface.h # IInputInjector interface
      ipc.h                      # IPC structures

  platform/
    windows/
      window_system_win32.h/.cpp
      input_hook_win32.h/.cpp
      input_injector_win32.h/.cpp
      hook.h                     # Windows hook utilities

    linux/
      window_system_linux.cpp    # Split across multiple files
      window_system_linux_queries.h
      window_system_linux_manipulation.h
      input_hook_linux.h/.cpp    # evdev-based hook
      input_injector_linux.cpp   # uinput-based injection
      keycode_mapping.h/.cpp     # evdev <-> YAMY mapping
      device_manager_linux.h/.cpp # evdev device discovery
```
