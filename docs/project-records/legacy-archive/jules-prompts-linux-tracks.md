# Jules Prompts for Linux Implementation Tracks

**Total Tracks:** 12
**Can run in parallel:** YES (zero conflicts)
**Integration order:** See linux-parallel-implementation-plan.md

---

## How to Use These Prompts

1. Copy each prompt below
2. Give to Jules (one per instance/session)
3. Jules will create PR
4. Review and merge PRs in integration order

Each prompt is **completely self-contained** with all necessary context.

---

# TRACK 1: X11 Window Queries (Basic)

## Jules Prompt

```
You are implementing Track 1 of the Linux porting project for YAMY (keyboard remapper).

## Your Task

Implement basic X11 window query functions for Linux.

## Context

YAMY is being ported to Linux. The Windows implementation is complete and uses a WindowSystem interface. You are implementing the Linux version using X11.

## Files to Read (for context)

1. `src/core/platform/window_system_interface.h` - Interface you're implementing
2. `src/core/platform/types.h` - Platform types (WindowHandle, Point, Rect, etc.)
3. `src/platform/linux/window_system_linux_queries.h` - Your interface header

## Files to Create

Create: `src/platform/linux/window_system_linux_queries.cpp`

## Implementation Requirements

Implement these 8 methods using X11 (Xlib):

1. `getForegroundWindow()` - Get the currently active/focused window
   - Use `_NET_ACTIVE_WINDOW` property from root window
   - X11 API: XGetInputFocus() or check root window property

2. `windowFromPoint(x, y)` - Get window at screen coordinates
   - X11 API: XQueryPointer() and XTranslateCoordinates()

3. `getWindowText(hwnd)` - Get window title
   - Use `_NET_WM_NAME` (UTF-8) or WM_NAME property
   - X11 API: XGetWindowProperty()

4. `getTitleName(hwnd)` - Same as getWindowText()

5. `getClassName(hwnd)` - Get window class name
   - Use `WM_CLASS` property
   - X11 API: XGetClassHint()

6. `getWindowThreadId(hwnd)` - Get window's thread ID
   - Use `_NET_WM_PID` property to get process ID
   - Return PID (Linux doesn't have separate thread ID per window)

7. `getWindowProcessId(hwnd)` - Get window's process ID
   - Use `_NET_WM_PID` property
   - X11 API: XGetWindowProperty()

8. `getWindowRect(hwnd, rect)` - Get window position and size
   - X11 API: XGetWindowAttributes() and XTranslateCoordinates()
   - Return screen coordinates, not relative

## Code Template

```cpp
#include "window_system_linux_queries.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cstring>

namespace yamy::platform {

// Helper: Get X11 display (create once, reuse)
static Display* getDisplay() {
    static Display* display = XOpenDisplay(nullptr);
    return display;
}

// Helper: Get atom by name
static Atom getAtom(const char* name) {
    Display* display = getDisplay();
    return XInternAtom(display, name, False);
}

// Helper: Get window property
static bool getWindowProperty(WindowHandle hwnd, Atom property,
                             Atom type, unsigned char** data,
                             unsigned long* items) {
    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    Atom actual_type;
    int actual_format;
    unsigned long bytes_after;

    int status = XGetWindowProperty(display, window, property, 0, (~0L),
                                    False, type, &actual_type, &actual_format,
                                    items, &bytes_after, data);

    return (status == Success && *items > 0);
}

WindowSystemLinuxQueries::WindowSystemLinuxQueries() {
    // Initialize X11 connection
    getDisplay();
}

WindowSystemLinuxQueries::~WindowSystemLinuxQueries() {
    // Don't close display (it's static)
}

WindowHandle WindowSystemLinuxQueries::getForegroundWindow() {
    Display* display = getDisplay();
    if (!display) return nullptr;

    // Method 1: Try _NET_ACTIVE_WINDOW
    Atom netActiveWindow = getAtom("_NET_ACTIVE_WINDOW");
    Window root = DefaultRootWindow(display);

    unsigned char* data = nullptr;
    unsigned long items = 0;

    if (getWindowProperty(reinterpret_cast<WindowHandle>(root),
                         netActiveWindow, XA_WINDOW,
                         &data, &items)) {
        Window* active = reinterpret_cast<Window*>(data);
        Window result = *active;
        XFree(data);
        return reinterpret_cast<WindowHandle>(result);
    }

    // Method 2: Fallback to XGetInputFocus
    Window focus;
    int revert_to;
    XGetInputFocus(display, &focus, &revert_to);

    return reinterpret_cast<WindowHandle>(focus);
}

WindowHandle WindowSystemLinuxQueries::windowFromPoint(const Point& pt) {
    Display* display = getDisplay();
    if (!display) return nullptr;

    Window root = DefaultRootWindow(display);
    Window child = root;
    int root_x = pt.x, root_y = pt.y;
    int win_x, win_y;
    unsigned int mask;

    // Query pointer to find window at position
    if (XQueryPointer(display, root, &root, &child,
                     &root_x, &root_y, &win_x, &win_y, &mask)) {
        // Traverse to find deepest window at position
        while (child != None) {
            Window temp_child;
            XTranslateCoordinates(display, root, child,
                                 pt.x, pt.y, &win_x, &win_y, &temp_child);
            if (temp_child == None) break;
            child = temp_child;
        }

        return reinterpret_cast<WindowHandle>(child);
    }

    return nullptr;
}

std::string WindowSystemLinuxQueries::getWindowText(WindowHandle hwnd) {
    if (!hwnd) return "";

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    // Try UTF-8 name first (_NET_WM_NAME)
    Atom netWmName = getAtom("_NET_WM_NAME");
    Atom utf8String = getAtom("UTF8_STRING");
    unsigned char* data = nullptr;
    unsigned long items = 0;

    if (getWindowProperty(hwnd, netWmName, utf8String, &data, &items)) {
        std::string result(reinterpret_cast<char*>(data));
        XFree(data);
        return result;
    }

    // Fallback to WM_NAME
    char* name = nullptr;
    if (XFetchName(display, window, &name)) {
        std::string result(name);
        XFree(name);
        return result;
    }

    return "";
}

std::string WindowSystemLinuxQueries::getTitleName(WindowHandle hwnd) {
    return getWindowText(hwnd);  // Same as window text
}

std::string WindowSystemLinuxQueries::getClassName(WindowHandle hwnd) {
    if (!hwnd) return "";

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    XClassHint class_hint;
    if (XGetClassHint(display, window, &class_hint)) {
        std::string result;
        if (class_hint.res_class) {
            result = class_hint.res_class;
        } else if (class_hint.res_name) {
            result = class_hint.res_name;
        }

        if (class_hint.res_name) XFree(class_hint.res_name);
        if (class_hint.res_class) XFree(class_hint.res_class);

        return result;
    }

    return "";
}

uint32_t WindowSystemLinuxQueries::getWindowThreadId(WindowHandle hwnd) {
    // Linux doesn't have per-window thread IDs
    // Return process ID instead (same as getWindowProcessId)
    return getWindowProcessId(hwnd);
}

uint32_t WindowSystemLinuxQueries::getWindowProcessId(WindowHandle hwnd) {
    if (!hwnd) return 0;

    Atom netWmPid = getAtom("_NET_WM_PID");
    unsigned char* data = nullptr;
    unsigned long items = 0;

    if (getWindowProperty(hwnd, netWmPid, XA_CARDINAL, &data, &items)) {
        uint32_t pid = *reinterpret_cast<uint32_t*>(data);
        XFree(data);
        return pid;
    }

    return 0;
}

bool WindowSystemLinuxQueries::getWindowRect(WindowHandle hwnd, Rect* rect) {
    if (!hwnd || !rect) return false;

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    XWindowAttributes attrs;
    if (!XGetWindowAttributes(display, window, &attrs)) {
        return false;
    }

    // Translate to screen coordinates
    Window child;
    int x, y;
    XTranslateCoordinates(display, window, attrs.root,
                         0, 0, &x, &y, &child);

    rect->left = x;
    rect->top = y;
    rect->right = x + attrs.width;
    rect->bottom = y + attrs.height;

    return true;
}

} // namespace yamy::platform
```

## Build Instructions

Add to CMakeLists.txt:
```cmake
target_link_libraries(yamy_platform_linux PUBLIC X11)
```

## Testing

Create test file:
```cpp
#include "window_system_linux_queries.h"
#include <iostream>

int main() {
    yamy::platform::WindowSystemLinuxQueries queries;

    auto hwnd = queries.getForegroundWindow();
    if (hwnd) {
        std::cout << "Active window: " << hwnd << std::endl;
        std::cout << "Title: " << queries.getWindowText(hwnd) << std::endl;
        std::cout << "Class: " << queries.getClassName(hwnd) << std::endl;
        std::cout << "PID: " << queries.getWindowProcessId(hwnd) << std::endl;

        yamy::platform::Rect rect;
        if (queries.getWindowRect(hwnd, &rect)) {
            std::cout << "Rect: " << rect.left << "," << rect.top
                     << " " << rect.right << "," << rect.bottom << std::endl;
        }
    }

    return 0;
}
```

Build and run:
```bash
g++ -o test_track1 test_track1.cpp \
    src/platform/linux/window_system_linux_queries.cpp \
    -lX11 -I src/
./test_track1
```

## PR Details

**Title:** `feat(linux): Track 1 - X11 window queries implementation`

**Description:**
```
Implements basic X11 window query functions for Linux platform:
- getForegroundWindow() - Get active window using _NET_ACTIVE_WINDOW
- windowFromPoint() - Window at screen coordinates
- getWindowText/getTitleName() - Window title via _NET_WM_NAME
- getClassName() - Window class via WM_CLASS
- getWindowProcessId() - Process ID via _NET_WM_PID
- getWindowRect() - Window position and size

Uses X11/Xlib APIs with EWMH (Extended Window Manager Hints) support.

Part of Track 1 (12 parallel tracks for Linux porting).

Tested with: X11 window manager
Dependencies: libX11
```

## Verification Checklist

- [ ] Code compiles without errors
- [ ] Code compiles without warnings
- [ ] All 8 methods implemented
- [ ] Test program runs successfully
- [ ] Can detect active window
- [ ] Can get window title
- [ ] Can get window class
- [ ] Can get window rectangle
- [ ] Follows code style (.clang-format)
- [ ] No memory leaks (XFree called for all allocations)

---

Please implement Track 1 and create a PR. Do not modify any other files.
```

---

# TRACK 2: X11 Window Manipulation

## Jules Prompt

```
You are implementing Track 2 of the Linux porting project for YAMY.

## Your Task

Implement X11 window manipulation functions (move, show, close, etc.).

## Context

This is Track 2 of 12 parallel tracks. Track 1 implements queries, you implement manipulation.

## Files to Read

1. `src/core/platform/window_system_interface.h` - Interface
2. `src/platform/linux/window_system_linux_manipulation.h` - Your header

## Files to Create

Create: `src/platform/linux/window_system_linux_manipulation.cpp`

## Methods to Implement (6)

1. `setForegroundWindow(hwnd)` - Activate/focus window
   - X11 API: XSetInputFocus() and raise window

2. `moveWindow(hwnd, rect)` - Move and resize window
   - X11 API: XMoveResizeWindow()

3. `showWindow(hwnd, cmdShow)` - Show/hide/minimize window
   - X11 API: XMapWindow(), XUnmapWindow(), XIconifyWindow()

4. `closeWindow(hwnd)` - Send close request to window
   - X11 API: Send WM_DELETE_WINDOW client message

5. `registerWindowMessage(name)` - Register custom message
   - X11 API: XInternAtom() to create atom

6. `sendMessageTimeout(hwnd, msg, ...)` - Send message with timeout
   - X11 API: XSendEvent() with custom atom

## Code Template

```cpp
#include "window_system_linux_manipulation.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

namespace yamy::platform {

static Display* getDisplay() {
    static Display* display = XOpenDisplay(nullptr);
    return display;
}

static Atom getAtom(const char* name) {
    return XInternAtom(getDisplay(), name, False);
}

WindowSystemLinuxManipulation::WindowSystemLinuxManipulation() {}
WindowSystemLinuxManipulation::~WindowSystemLinuxManipulation() {}

bool WindowSystemLinuxManipulation::setForegroundWindow(WindowHandle hwnd) {
    if (!hwnd) return false;

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    // Raise window
    XRaiseWindow(display, window);

    // Set input focus
    XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);

    // Send _NET_ACTIVE_WINDOW message
    XEvent event;
    memset(&event, 0, sizeof(event));
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = getAtom("_NET_ACTIVE_WINDOW");
    event.xclient.format = 32;
    event.xclient.data.l[0] = 2;  // Source: pager
    event.xclient.data.l[1] = CurrentTime;

    Window root = DefaultRootWindow(display);
    XSendEvent(display, root, False,
               SubstructureNotifyMask | SubstructureRedirectMask,
               &event);

    XFlush(display);
    return true;
}

bool WindowSystemLinuxManipulation::moveWindow(WindowHandle hwnd,
                                               const Rect& rect) {
    if (!hwnd) return false;

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    int x = rect.left;
    int y = rect.top;
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    XMoveResizeWindow(display, window, x, y, width, height);
    XFlush(display);

    return true;
}

bool WindowSystemLinuxManipulation::showWindow(WindowHandle hwnd,
                                               int cmdShow) {
    if (!hwnd) return false;

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    // cmdShow values (Windows compatible):
    // 0 = SW_HIDE, 1 = SW_SHOWNORMAL, 3 = SW_MAXIMIZE, 6 = SW_MINIMIZE

    switch (cmdShow) {
        case 0: // Hide
            XUnmapWindow(display, window);
            break;

        case 6: // Minimize
            XIconifyWindow(display, window, DefaultScreen(display));
            break;

        case 1: // Show normal
        case 3: // Maximize (TODO: set _NET_WM_STATE)
        default:
            XMapWindow(display, window);
            XRaiseWindow(display, window);
            break;
    }

    XFlush(display);
    return true;
}

bool WindowSystemLinuxManipulation::closeWindow(WindowHandle hwnd) {
    if (!hwnd) return false;

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    // Send WM_DELETE_WINDOW protocol message
    Atom wmProtocols = getAtom("WM_PROTOCOLS");
    Atom wmDeleteWindow = getAtom("WM_DELETE_WINDOW");

    XEvent event;
    memset(&event, 0, sizeof(event));
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = wmProtocols;
    event.xclient.format = 32;
    event.xclient.data.l[0] = wmDeleteWindow;
    event.xclient.data.l[1] = CurrentTime;

    XSendEvent(display, window, False, NoEventMask, &event);
    XFlush(display);

    return true;
}

uint32_t WindowSystemLinuxManipulation::registerWindowMessage(
    const std::string& name) {
    // In X11, messages are atoms
    Atom atom = XInternAtom(getDisplay(), name.c_str(), False);
    return static_cast<uint32_t>(atom);
}

bool WindowSystemLinuxManipulation::sendMessageTimeout(
    WindowHandle hwnd, uint32_t msg,
    uintptr_t wParam, intptr_t lParam,
    uint32_t flags, uint32_t timeout,
    uintptr_t* result) {

    if (!hwnd) return false;

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    // Send custom client message
    XEvent event;
    memset(&event, 0, sizeof(event));
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = static_cast<Atom>(msg);
    event.xclient.format = 32;
    event.xclient.data.l[0] = wParam;
    event.xclient.data.l[1] = lParam;

    Status status = XSendEvent(display, window, False,
                              NoEventMask, &event);
    XFlush(display);

    if (result) *result = 0;
    return (status != 0);
}

} // namespace yamy::platform
```

## Testing

```cpp
#include "window_system_linux_manipulation.h"
#include <iostream>

int main() {
    yamy::platform::WindowSystemLinuxManipulation manip;

    // Get some window handle (from Track 1)
    WindowHandle hwnd = ...; // Use Track 1 to get active window

    // Test set foreground
    if (manip.setForegroundWindow(hwnd)) {
        std::cout << "✅ setForegroundWindow works" << std::endl;
    }

    // Test move
    Rect newRect{100, 100, 600, 500};
    if (manip.moveWindow(hwnd, newRect)) {
        std::cout << "✅ moveWindow works" << std::endl;
    }

    return 0;
}
```

## PR Details

**Title:** `feat(linux): Track 2 - X11 window manipulation`

**Description:**
```
Implements X11 window manipulation for Linux:
- setForegroundWindow() - Activate window
- moveWindow() - Move/resize window
- showWindow() - Show/hide/minimize
- closeWindow() - Send WM_DELETE_WINDOW
- registerWindowMessage() - Create custom atom
- sendMessageTimeout() - Send client message

Track 2 of 12 parallel tracks.
```

---

Please implement Track 2 and create a PR.
```

---

## Additional Tracks (3-12)

Due to length, I'll provide abbreviated versions. Full prompts follow the same pattern.

### TRACK 3: Window Hierarchy (3 hours)
- getParent(), isMDIChild(), isChild()
- getToplevelWindow(), isConsoleWindow()
- Uses XQueryTree() for hierarchy

### TRACK 4: Mouse & Cursor (2 hours)
- getCursorPos(), setCursorPos()
- Uses XQueryPointer(), XWarpPointer()

### TRACK 5: Monitor Support (3 hours)
- getMonitorFromWindow(), getMonitorRect()
- Uses XRandR extension

### TRACK 6: POSIX Synchronization (4 hours)
- waitForObject() with pthread/semaphore
- File: sync_linux.cpp

### TRACK 7: Unix IPC (4 hours)
- sendCopyData() with Unix domain sockets
- File: ipc_linux.cpp

### TRACK 8: Hook Data (1 hour)
- getHookData() - global instance
- File: hook_data_linux.cpp

### TRACK 9: evdev Input Capture (8 hours)
- Device enumeration with libudev
- Read events from /dev/input/eventX
- File: input_hook_linux.cpp, device_manager_linux.cpp

### TRACK 10: uinput Injection (6 hours)
- Create virtual keyboard
- Inject key events
- File: input_injector_linux.cpp

### TRACK 11: Key Mapping (5 hours)
- evdev keycode ↔ YAMY keycode translation
- File: keycode_mapping.cpp

### TRACK 12: Thread Management (2 hours)
- pthread wrapper
- File: thread_linux.cpp

---

## Summary

**12 Tracks** = Maximum parallelization
**Zero conflicts** = Each track different files
**Clear interfaces** = All defined in advance
**Independent testing** = Each track testable alone
**46 hours sequential** → **~8 hours parallel with 12 agents**

Would you like me to generate the complete detailed prompts for tracks 3-12?
