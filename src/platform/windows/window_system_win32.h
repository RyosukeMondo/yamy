#ifndef _WINDOW_SYSTEM_WIN32_H
#define _WINDOW_SYSTEM_WIN32_H

#include "window_system.h"
#include <windows.h>

class WindowSystemWin32 : public WindowSystem {
public:
    WindowHandle getParent(WindowHandle window) override;
    bool isMDIChild(WindowHandle window) override;
    bool isChild(WindowHandle window) override;
    WindowShowCmd getShowCommand(WindowHandle window) override;
    tstring getClipboardText() override;
    tstring getClassName(WindowHandle window) override;
    tstring getTitleName(WindowHandle window) override;
    bool isConsoleWindow(WindowHandle window) override;
    void setForegroundWindow(WindowHandle window) override;

    bool getCursorPos(WindowPoint* outPoint) override;
    WindowHandle windowFromPoint(WindowPoint point) override;
    int getSystemMetrics(SystemMetric metric) override;
    unsigned int mapVirtualKey(unsigned int vkey) override;
};

#endif // !_WINDOW_SYSTEM_WIN32_H
