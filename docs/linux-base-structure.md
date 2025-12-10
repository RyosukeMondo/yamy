# Linux Implementation Base Structure

**Purpose:** Set up foundation before launching parallel tracks
**Do this FIRST before asking Jules to implement tracks**

---

## Step 1: Create Directory Structure

```bash
mkdir -p src/platform/linux
touch src/platform/linux/.gitkeep
```

Already exists, verify:
```bash
ls -la src/platform/linux/
```

---

## Step 2: Create Main WindowSystem Skeleton

This file will delegate to track-specific implementations.

**File:** `src/platform/linux/window_system_linux_main.cpp`

```cpp
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window_system_linux_main.cpp - Main WindowSystem implementation for Linux
// This file delegates to track-specific implementations

#include "core/platform/window_system_interface.h"
#include <memory>

namespace yamy::platform {

// Forward declarations for track implementations
class WindowSystemLinuxQueries;
class WindowSystemLinuxManipulation;
class WindowSystemLinuxHierarchy;
class WindowSystemLinuxMouse;
class WindowSystemLinuxMonitor;

class WindowSystemLinux : public IWindowSystem {
private:
    // Track implementations
    std::unique_ptr<WindowSystemLinuxQueries> m_queries;
    std::unique_ptr<WindowSystemLinuxManipulation> m_manipulation;
    std::unique_ptr<WindowSystemLinuxHierarchy> m_hierarchy;
    std::unique_ptr<WindowSystemLinuxMouse> m_mouse;
    std::unique_ptr<WindowSystemLinuxMonitor> m_monitor;

public:
    WindowSystemLinux();
    ~WindowSystemLinux() override;

    // Delegate to track implementations
    // (Track implementations will be linked in)

    // Track 1: Queries
    WindowHandle getForegroundWindow() override;
    WindowHandle windowFromPoint(const Point& pt) override;
    std::string getWindowText(WindowHandle hwnd) override;
    std::string getTitleName(WindowHandle hwnd) override;
    std::string getClassName(WindowHandle hwnd) override;
    uint32_t getWindowThreadId(WindowHandle hwnd) override;
    uint32_t getWindowProcessId(WindowHandle hwnd) override;
    bool getWindowRect(WindowHandle hwnd, Rect* rect) override;

    // Track 2: Manipulation
    bool setForegroundWindow(WindowHandle hwnd) override;
    bool moveWindow(WindowHandle hwnd, const Rect& rect) override;
    bool showWindow(WindowHandle hwnd, int cmdShow) override;
    bool closeWindow(WindowHandle hwnd) override;
    uint32_t registerWindowMessage(const std::string& name) override;
    bool sendMessageTimeout(WindowHandle hwnd, uint32_t msg,
                           uintptr_t wParam, intptr_t lParam,
                           uint32_t flags, uint32_t timeout,
                           uintptr_t* result) override;

    // Track 3: Hierarchy
    WindowHandle getParent(WindowHandle window) override;
    bool isMDIChild(WindowHandle window) override;
    bool isChild(WindowHandle window) override;
    WindowShowCmd getShowCommand(WindowHandle window) override;
    bool isConsoleWindow(WindowHandle window) override;
    WindowHandle getToplevelWindow(WindowHandle hwnd, bool* isMDI) override;

    // Track 4: Mouse
    void getCursorPos(Point* pt) override;
    void setCursorPos(const Point& pt) override;

    // Track 5: Monitor
    MonitorHandle getMonitorFromWindow(WindowHandle hwnd) override;
    MonitorHandle getMonitorFromPoint(const Point& pt) override;
    bool getMonitorRect(MonitorHandle monitor, Rect* rect) override;
    bool getMonitorWorkArea(MonitorHandle monitor, Rect* rect) override;
    MonitorHandle getPrimaryMonitor() override;

    // Track 7: IPC
    bool sendCopyData(WindowHandle sender, WindowHandle target,
                     const CopyData& data, uint32_t flags,
                     uint32_t timeout_ms, uintptr_t* result) override;

    // Other methods (lower priority)
    bool changeMessageFilter(uint32_t message, uint32_t action) override;
    void* openMutex(const std::string& name) override;
    void* openFileMapping(const std::string& name) override;
    void* mapViewOfFile(void* handle) override;
    bool unmapViewOfFile(void* addr) override;
    void closeHandle(void* handle) override;
    bool postMessage(WindowHandle hwnd, uint32_t msg,
                    uintptr_t wParam, intptr_t lParam) override;
};

// Factory function
IWindowSystem* createWindowSystem() {
    return new WindowSystemLinux();
}

} // namespace yamy::platform
```

---

## Step 3: Create Track Header Templates

These headers define the interfaces for each track's implementation.

### Track 1: Queries

**File:** `src/platform/linux/window_system_linux_queries.h`

```cpp
#pragma once
#include "core/platform/types.h"
#include <string>

namespace yamy::platform {

class WindowSystemLinuxQueries {
public:
    WindowSystemLinuxQueries();
    ~WindowSystemLinuxQueries();

    WindowHandle getForegroundWindow();
    WindowHandle windowFromPoint(const Point& pt);
    std::string getWindowText(WindowHandle hwnd);
    std::string getTitleName(WindowHandle hwnd);
    std::string getClassName(WindowHandle hwnd);
    uint32_t getWindowThreadId(WindowHandle hwnd);
    uint32_t getWindowProcessId(WindowHandle hwnd);
    bool getWindowRect(WindowHandle hwnd, Rect* rect);
};

} // namespace yamy::platform
```

### Track 2: Manipulation

**File:** `src/platform/linux/window_system_linux_manipulation.h`

```cpp
#pragma once
#include "core/platform/types.h"
#include <string>

namespace yamy::platform {

class WindowSystemLinuxManipulation {
public:
    WindowSystemLinuxManipulation();
    ~WindowSystemLinuxManipulation();

    bool setForegroundWindow(WindowHandle hwnd);
    bool moveWindow(WindowHandle hwnd, const Rect& rect);
    bool showWindow(WindowHandle hwnd, int cmdShow);
    bool closeWindow(WindowHandle hwnd);
    uint32_t registerWindowMessage(const std::string& name);
    bool sendMessageTimeout(WindowHandle hwnd, uint32_t msg,
                           uintptr_t wParam, intptr_t lParam,
                           uint32_t flags, uint32_t timeout,
                           uintptr_t* result);
};

} // namespace yamy::platform
```

### Track 3: Hierarchy

**File:** `src/platform/linux/window_system_linux_hierarchy.h`

```cpp
#pragma once
#include "core/platform/types.h"

namespace yamy::platform {

enum class WindowShowCmd; // Forward declare

class WindowSystemLinuxHierarchy {
public:
    WindowSystemLinuxHierarchy();
    ~WindowSystemLinuxHierarchy();

    WindowHandle getParent(WindowHandle window);
    bool isMDIChild(WindowHandle window);
    bool isChild(WindowHandle window);
    WindowShowCmd getShowCommand(WindowHandle window);
    bool isConsoleWindow(WindowHandle window);
    WindowHandle getToplevelWindow(WindowHandle hwnd, bool* isMDI);
};

} // namespace yamy::platform
```

### Track 4-5: Mouse and Monitor (similar pattern)

---

## Step 4: Create CMakeLists.txt Template

**File:** `src/platform/linux/CMakeLists.txt`

```cmake
# Linux platform implementation

# Find required libraries
find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(XRANDR REQUIRED xrandr)
pkg_check_modules(UDEV REQUIRED libudev)

# Source files (will be populated by tracks)
set(LINUX_PLATFORM_SOURCES
    window_system_linux_main.cpp
    # Track 1
    window_system_linux_queries.cpp
    # Track 2
    window_system_linux_manipulation.cpp
    # Track 3
    window_system_linux_hierarchy.cpp
    # Track 4
    window_system_linux_mouse.cpp
    # Track 5
    window_system_linux_monitor.cpp
    # Track 6
    sync_linux.cpp
    # Track 7
    ipc_linux.cpp
    # Track 8
    hook_data_linux.cpp
    # Track 9
    input_hook_linux.cpp
    device_manager_linux.cpp
    # Track 10
    input_injector_linux.cpp
    # Track 11
    keycode_mapping.cpp
    # Track 12
    thread_linux.cpp
)

# Platform library
add_library(yamy_platform_linux STATIC ${LINUX_PLATFORM_SOURCES})

target_include_directories(yamy_platform_linux PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${X11_INCLUDE_DIR}
    ${XRANDR_INCLUDE_DIRS}
    ${UDEV_INCLUDE_DIRS}
)

target_link_libraries(yamy_platform_linux PUBLIC
    ${X11_LIBRARIES}
    ${XRANDR_LIBRARIES}
    ${UDEV_LIBRARIES}
    pthread
)

# Export for main build
set(YAMY_PLATFORM_LIBRARY yamy_platform_linux PARENT_SCOPE)
```

---

## Step 5: Create Build Configuration

Update root CMakeLists.txt to include Linux platform:

```cmake
# In root CMakeLists.txt
if(UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux")
    add_subdirectory(src/platform/linux)
    set(PLATFORM_LIBRARIES ${YAMY_PLATFORM_LIBRARY})
endif()
```

---

## Step 6: Create Test Harness Template

**File:** `src/platform/linux/test_track.cpp`

```cpp
// Template for testing individual tracks
#include <iostream>
#include "core/platform/window_system_interface.h"

using namespace yamy::platform;

int main() {
    std::cout << "=== Track Test Harness ===" << std::endl;

    // Create WindowSystem
    WindowSystemLinux ws;

    // TODO: Add track-specific tests here

    std::cout << "=== Test Complete ===" << std::endl;
    return 0;
}
```

---

## Step 7: Create .clang-format (Code Style)

**File:** `src/platform/linux/.clang-format`

```yaml
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
PointerAlignment: Left
```

This ensures consistent formatting across all tracks.

---

## Summary of Base Structure

After completing these steps, you'll have:

```
src/platform/linux/
├── CMakeLists.txt                          (build config)
├── .clang-format                           (code style)
├── window_system_linux_main.cpp            (main delegator)
├── window_system_linux_queries.h           (Track 1 interface)
├── window_system_linux_manipulation.h      (Track 2 interface)
├── window_system_linux_hierarchy.h         (Track 3 interface)
├── test_track.cpp                          (test harness)
└── (tracks will add implementation files)
```

---

## Verification

After creating base structure, verify:

```bash
# Check files exist
ls src/platform/linux/*.h
ls src/platform/linux/*.cpp

# Try to configure build (will fail for now, but should recognize structure)
cmake -B build
```

---

## Ready for Parallel Implementation

Once base structure is in place:
- ✅ Each track has clear interface
- ✅ Each track knows what file to create
- ✅ Build system ready
- ✅ No conflicts possible

Now ready to generate Jules prompts!
