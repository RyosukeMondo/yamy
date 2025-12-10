#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window_system_linux_queries.h - Basic window query functions (Track 1)

#include "../../core/platform/types.h"
#include <string>

namespace yamy::platform {

/// Track 1: Basic window queries using X11
class WindowSystemLinuxQueries {
public:
    WindowSystemLinuxQueries();
    ~WindowSystemLinuxQueries();

    /// Get the currently active/focused window
    WindowHandle getForegroundWindow();

    /// Get window at screen coordinates
    WindowHandle windowFromPoint(const Point& pt);

    /// Get window title
    std::string getWindowText(WindowHandle hwnd);

    /// Get window title (same as getWindowText)
    std::string getTitleName(WindowHandle hwnd);

    /// Get window class name
    std::string getClassName(WindowHandle hwnd);

    /// Get window's thread ID
    uint32_t getWindowThreadId(WindowHandle hwnd);

    /// Get window's process ID
    uint32_t getWindowProcessId(WindowHandle hwnd);

    /// Get window position and size
    bool getWindowRect(WindowHandle hwnd, Rect* rect);
};

} // namespace yamy::platform
