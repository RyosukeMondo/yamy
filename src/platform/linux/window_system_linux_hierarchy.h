#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window_system_linux_hierarchy.h - Window hierarchy (Track 3)

#include "../../core/platform/types.h"

namespace yamy::platform {

enum class WindowShowCmd; // Forward declaration

/// Track 3: Window hierarchy using X11
class WindowSystemLinuxHierarchy {
public:
    WindowSystemLinuxHierarchy();
    ~WindowSystemLinuxHierarchy();

    /// Get parent window
    WindowHandle getParent(WindowHandle window);

    /// Check if window is MDI child
    bool isMDIChild(WindowHandle window);

    /// Check if window is a child window
    bool isChild(WindowHandle window);

    /// Get window show state
    WindowShowCmd getShowCommand(WindowHandle window);

    /// Check if window is a console/terminal
    bool isConsoleWindow(WindowHandle window);

    /// Get top-level window
    WindowHandle getToplevelWindow(WindowHandle hwnd, bool* isMDI);
};

} // namespace yamy::platform
