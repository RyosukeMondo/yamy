#include "window_system_win32.h"
#include "../utils/stringtool.h"
#include "windowstool.h"
#include <vector>
#include <tchar.h>

#ifdef UNICODE
#define CF_TTEXT CF_UNICODETEXT
#else
#define CF_TTEXT CF_TEXT
#endif

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
    HGLOBAL hdata;
    const _TCHAR* text = clipboardGetText(&hdata);
    tstring result;
    if (text) {
        result = text;
    }
    clipboardClose(hdata);
    return result;
}

bool WindowSystemWin32::setClipboardText(const tstring& text) {
    if (!OpenClipboard(NULL))
        return false;

    HGLOBAL hdataNew = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                                   (text.size() + 1) * sizeof(_TCHAR));
    if (!hdataNew) {
        CloseClipboard();
        return false;
    }
    _TCHAR* dataNew = reinterpret_cast<_TCHAR*>(GlobalLock(hdataNew));
    _tcscpy(dataNew, text.c_str());
    GlobalUnlock(hdataNew);
    
    EmptyClipboard();
    bool result = SetClipboardData(CF_TTEXT, hdataNew) != NULL;
    
    clipboardClose(NULL, result ? NULL : hdataNew); 
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

bool WindowSystemWin32::setCursorPos(int x, int y) {
    return SetCursorPos(x, y) != 0;
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

bool WindowSystemWin32::getWindowRect(WindowHandle window, WindowRect* outRect) {
    RECT rc;
    if (GetWindowRect((HWND)window, &rc)) {
        outRect->left = rc.left;
        outRect->top = rc.top;
        outRect->right = rc.right;
        outRect->bottom = rc.bottom;
        return true;
    }
    return false;
}

bool WindowSystemWin32::getClientRect(WindowHandle window, WindowRect* outRect) {
    RECT rc;
    if (GetClientRect((HWND)window, &rc)) {
        outRect->left = rc.left;
        outRect->top = rc.top;
        outRect->right = rc.right;
        outRect->bottom = rc.bottom;
        return true;
    }
    return false;
}

bool WindowSystemWin32::getChildWindowRect(WindowHandle window, WindowRect* outRect) {
    RECT rc;
    if (!GetWindowRect((HWND)window, &rc))
        return false;
    POINT p = { rc.left, rc.top };
    HWND phwnd = GetParent((HWND)window);
    if (!phwnd)
        return false;
    if (!ScreenToClient(phwnd, &p))
        return false;
    outRect->left = p.x;
    outRect->top = p.y;
    p.x = rc.right;
    p.y = rc.bottom;
    ScreenToClient(phwnd, &p);
    outRect->right = p.x;
    outRect->bottom = p.y;
    return true;
}

bool WindowSystemWin32::getWorkArea(WindowRect* outRect) {
    RECT rc;
    if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0)) {
        outRect->left = rc.left;
        outRect->top = rc.top;
        outRect->right = rc.right;
        outRect->bottom = rc.bottom;
        return true;
    }
    return false;
}

bool WindowSystemWin32::postMessage(WindowHandle window, unsigned int message, uintptr_t wParam, intptr_t lParam) {
    return PostMessage((HWND)window, message, (WPARAM)wParam, (LPARAM)lParam) != 0;
}

unsigned int WindowSystemWin32::registerWindowMessage(const tstring& name) {
    return RegisterWindowMessage(name.c_str());
}

bool WindowSystemWin32::disconnectNamedPipe(void* handle) {
    return DisconnectNamedPipe((HANDLE)handle) != 0;
}

bool WindowSystemWin32::connectNamedPipe(void* handle, void* overlapped) {
    return ConnectNamedPipe((HANDLE)handle, (LPOVERLAPPED)overlapped) != 0;
}

bool WindowSystemWin32::writeFile(void* handle, const void* buffer, unsigned int bytesToWrite, unsigned int* bytesWritten, void* overlapped) {
    return WriteFile((HANDLE)handle, buffer, (DWORD)bytesToWrite, (LPDWORD)bytesWritten, (LPOVERLAPPED)overlapped) != 0;
}
