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
    bool setClipboardText(const tstring& text) override;
    tstring getClassName(WindowHandle window) override;
    tstring getTitleName(WindowHandle window) override;
    bool isConsoleWindow(WindowHandle window) override;
    void setForegroundWindow(WindowHandle window) override;

    bool getCursorPos(WindowPoint* outPoint) override;
    bool setCursorPos(int x, int y) override;
    WindowHandle windowFromPoint(WindowPoint point) override;
    int getSystemMetrics(SystemMetric metric) override;
    unsigned int mapVirtualKey(unsigned int vkey) override;

    bool getWindowRect(WindowHandle window, WindowRect* outRect) override;
    bool getClientRect(WindowHandle window, WindowRect* outRect) override;
    bool getChildWindowRect(WindowHandle window, WindowRect* outRect) override;
    bool getWorkArea(WindowRect* outRect) override;
    bool postMessage(WindowHandle window, unsigned int message, uintptr_t wParam, intptr_t lParam) override;
    unsigned int registerWindowMessage(const tstring& name) override;

    bool disconnectNamedPipe(void* handle) override;
    bool connectNamedPipe(void* handle, void* overlapped) override;
    bool writeFile(void* handle, const void* buffer, unsigned int bytesToWrite, unsigned int* bytesWritten, void* overlapped) override;
};

#endif // !_WINDOW_SYSTEM_WIN32_H
