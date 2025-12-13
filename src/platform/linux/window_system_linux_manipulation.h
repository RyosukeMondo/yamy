#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window_system_linux_manipulation.h - Window manipulation (Track 2)

#include "../../core/platform/types.h"
#include <string>

namespace yamy::platform {

/// Track 2: Window manipulation using X11
class WindowSystemLinuxManipulation {
public:
    WindowSystemLinuxManipulation();
    ~WindowSystemLinuxManipulation();

    /// Activate/focus window
    bool setForegroundWindow(WindowHandle hwnd);

    /// Move and resize window
    bool moveWindow(WindowHandle hwnd, const Rect& rect);

    /// Show/hide/minimize window
    bool showWindow(WindowHandle hwnd, int cmdShow);

    /// Send close request to window
    bool closeWindow(WindowHandle hwnd);

    /// Register custom message (create atom)
    uint32_t registerWindowMessage(const std::string& name);

    /// Send message with timeout
    bool sendMessageTimeout(WindowHandle hwnd, uint32_t msg,
                           uintptr_t wParam, intptr_t lParam,
                           uint32_t flags, uint32_t timeout,
                           uintptr_t* result);
};

} // namespace yamy::platform
