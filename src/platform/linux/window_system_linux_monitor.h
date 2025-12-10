#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window_system_linux_monitor.h - Monitor support (Track 5)

#include "../../core/platform/types.h"

namespace yamy::platform {

using MonitorHandle = void*;

/// Track 5: Multi-monitor support using XRandR
class WindowSystemLinuxMonitor {
public:
    WindowSystemLinuxMonitor();
    ~WindowSystemLinuxMonitor();

    /// Get monitor containing window
    MonitorHandle getMonitorFromWindow(WindowHandle hwnd);

    /// Get monitor at point
    MonitorHandle getMonitorFromPoint(const Point& pt);

    /// Get monitor dimensions
    bool getMonitorRect(MonitorHandle monitor, Rect* rect);

    /// Get monitor work area (minus panels/taskbar)
    bool getMonitorWorkArea(MonitorHandle monitor, Rect* rect);

    /// Get primary monitor
    MonitorHandle getPrimaryMonitor();
};

} // namespace yamy::platform
