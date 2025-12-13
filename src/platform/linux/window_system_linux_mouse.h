#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window_system_linux_mouse.h - Mouse and cursor (Track 4)

#include "../../core/platform/types.h"

namespace yamy::platform {

/// Track 4: Mouse and cursor using X11
class WindowSystemLinuxMouse {
public:
    WindowSystemLinuxMouse();
    ~WindowSystemLinuxMouse();

    /// Get cursor position
    void getCursorPos(Point* pt);

    /// Set cursor position
    void setCursorPos(const Point& pt);
};

} // namespace yamy::platform
