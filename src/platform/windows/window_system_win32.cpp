#include "window_system_win32.h"
#include "../utils/stringtool.h"
#include "windowstool.h"
#include <vector>
#include <tchar.h>

WindowSystem::WindowHandle WindowSystemWin32::getParent(WindowHandle window) {
    return (WindowHandle)GetParent((HWND)window);
}

bool WindowSystemWin32::isMDIChild(WindowHandle window) {
    HWND hwnd = (HWND)window;
    if (!hwnd) return false;
#ifdef MAYU64
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
#else
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
#endif
    return (exStyle & WS_EX_MDICHILD) != 0;
}

bool WindowSystemWin32::isChild(WindowHandle window) {
    HWND hwnd = (HWND)window;
    if (!hwnd) return false;
#ifdef MAYU64
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
#else
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
#endif
    return (style & WS_CHILD) != 0;
}

WindowShowCmd WindowSystemWin32::getShowCommand(WindowHandle window) {
    HWND hwnd = (HWND)window;
    WINDOWPLACEMENT placement;
    placement.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hwnd, &placement)) {
        switch (placement.showCmd) {
        case SW_SHOWMAXIMIZED:
            return WindowShowCmd::Maximized;
        case SW_SHOWMINIMIZED:
            return WindowShowCmd::Minimized;
        case SW_SHOWNORMAL:
            return WindowShowCmd::Normal;
        default:
            return WindowShowCmd::Normal;
        }
    }
    return WindowShowCmd::Unknown;
}

tstring WindowSystemWin32::getClipboardText() {
    HGLOBAL h;
    const _TCHAR *text = clipboardGetText(&h);
    tstring result = (text) ? text : _T("");
    clipboardClose(h);
    return result;
}

tstring WindowSystemWin32::getClassName(WindowHandle window) {
    HWND hwnd = (HWND)window;
    _TCHAR className[256]; 
    if (GetClassName(hwnd, className, 256)) {
        return className;
    }
    return _T("");
}

tstring WindowSystemWin32::getTitleName(WindowHandle window) {
    HWND hwnd = (HWND)window;
    int len = GetWindowTextLength(hwnd);
    if (len > 0) {
        std::vector<_TCHAR> buf(len + 1);
        GetWindowText(hwnd, &buf[0], len + 1);
        return &buf[0];
    }
    return _T("");
}

bool WindowSystemWin32::isConsoleWindow(WindowHandle window) {
    return _tcsicmp(getClassName(window).c_str(), _T("ConsoleWindowClass")) == 0;
}

void WindowSystemWin32::setForegroundWindow(WindowHandle window) {
    SetForegroundWindow((HWND)window);
}

bool WindowSystemWin32::getCursorPos(WindowPoint* outPoint) {
    POINT pt;
    if (GetCursorPos(&pt)) {
        outPoint->x = pt.x;
        outPoint->y = pt.y;
        return true;
    }
    return false;
}

WindowSystem::WindowHandle WindowSystemWin32::windowFromPoint(WindowPoint point) {
    POINT pt;
    pt.x = point.x;
    pt.y = point.y;
    return (WindowHandle)WindowFromPoint(pt);
}

int WindowSystemWin32::getSystemMetrics(SystemMetric metric) {
    int index = 0;
    switch (metric) {
        case SystemMetric::VirtualScreenWidth: index = SM_CXVIRTUALSCREEN; break;
        case SystemMetric::VirtualScreenHeight: index = SM_CYVIRTUALSCREEN; break;
        case SystemMetric::ScreenWidth: index = SM_CXSCREEN; break;
        case SystemMetric::ScreenHeight: index = SM_CYSCREEN; break;
        default: return 0;
    }
    return GetSystemMetrics(index);
}

unsigned int WindowSystemWin32::mapVirtualKey(unsigned int vkey) {
    return MapVirtualKey(vkey, 0);
}
