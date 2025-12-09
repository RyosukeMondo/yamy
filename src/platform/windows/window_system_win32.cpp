#include "window_system_win32.h"
#include "../../utils/stringtool.h"
#include "windowstool.h"
#include <vector>
#include <tchar.h>
#include <shellapi.h>

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
    if (!OpenClipboard(nullptr))
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
    bool result = SetClipboardData(CF_TTEXT, hdataNew) != nullptr;
    
    clipboardClose(nullptr, result ? nullptr : hdataNew); 
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

bool WindowSystemWin32::setWindowZOrder(WindowHandle window, ZOrder order) {
    HWND hwnd = (HWND)window;
    HWND hWndInsertAfter = HWND_TOP;
    UINT flags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE;
    
    switch (order) {
        case ZOrder::Top: 
            hWndInsertAfter = HWND_TOP; 
            flags |= SWP_ASYNCWINDOWPOS;
            break;
        case ZOrder::Bottom: 
            hWndInsertAfter = HWND_BOTTOM; 
            flags |= SWP_ASYNCWINDOWPOS;
            break;
        case ZOrder::TopMost: 
            hWndInsertAfter = HWND_TOPMOST; 
            break;
        case ZOrder::NoTopMost: 
            hWndInsertAfter = HWND_NOTOPMOST; 
            break;
    }
    return SetWindowPos(hwnd, hWndInsertAfter, 0, 0, 0, 0, flags) != 0;
}

bool WindowSystemWin32::isWindowTopMost(WindowHandle window) {
    HWND hwnd = (HWND)window;
#ifdef MAYU64
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
#else
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
#endif
    return (exStyle & WS_EX_TOPMOST) != 0;
}

bool WindowSystemWin32::isWindowLayered(WindowHandle window) {
    HWND hwnd = (HWND)window;
#ifdef MAYU64
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
#else
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
#endif
    return (exStyle & WS_EX_LAYERED) != 0;
}

bool WindowSystemWin32::setWindowLayered(WindowHandle window, bool enable) {
    HWND hwnd = (HWND)window;
#ifdef MAYU64
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
#else
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
#endif
    if (enable) {
        if (exStyle & WS_EX_LAYERED) return true; // already layered
#ifdef MAYU64
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
#else
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
#endif
    } else {
        if (!(exStyle & WS_EX_LAYERED)) return true; // already not layered
#ifdef MAYU64
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_LAYERED);
#else
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_LAYERED);
#endif
    }
    return true;
}

bool WindowSystemWin32::setLayeredWindowAttributes(WindowHandle window, unsigned long crKey, unsigned char bAlpha, unsigned long dwFlags) {
    return SetLayeredWindowAttributes((HWND)window, (COLORREF)crKey, (BYTE)bAlpha, (DWORD)dwFlags) != 0;
}

bool WindowSystemWin32::redrawWindow(WindowHandle window) {
    return RedrawWindow((HWND)window, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN) != 0;
}

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    WindowSystem::WindowEnumCallback* callback = (WindowSystem::WindowEnumCallback*)lParam;
    return (*callback)((WindowSystem::WindowHandle)hwnd) ? TRUE : FALSE;
}

bool WindowSystemWin32::enumerateWindows(WindowEnumCallback callback) {
    return EnumWindows(EnumWindowsProc, (LPARAM)&callback) != 0;
}

int WindowSystemWin32::shellExecute(const tstring& operation, const tstring& file, const tstring& parameters, const tstring& directory, int showCmd) {
    return (int)(INT_PTR)ShellExecute(
        nullptr,
        operation.empty() ? nullptr : operation.c_str(),
        file.empty() ? nullptr : file.c_str(),
        parameters.empty() ? nullptr : parameters.c_str(),
        directory.empty() ? nullptr : directory.c_str(),
        showCmd);
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

void* WindowSystemWin32::openMutex(const tstring& name) {
    return OpenMutex(MUTEX_ALL_ACCESS, FALSE, name.c_str());
}

void* WindowSystemWin32::openFileMapping(const tstring& name) {
    return OpenFileMapping(FILE_MAP_READ, FALSE, name.c_str());
}

void* WindowSystemWin32::mapViewOfFile(void* handle) {
    return MapViewOfFile((HANDLE)handle, FILE_MAP_READ, 0, 0, 0);
}

bool WindowSystemWin32::unmapViewOfFile(void* address) {
    return UnmapViewOfFile(address) != 0;
}

void WindowSystemWin32::closeHandle(void* handle) {
    CloseHandle((HANDLE)handle);
}

bool WindowSystemWin32::sendMessageTimeout(WindowHandle window, unsigned int msg, uintptr_t wParam, intptr_t lParam, unsigned int flags, unsigned int timeout, uintptr_t* result) {
#ifdef MAYU64
    DWORD_PTR res = 0;
#else
    DWORD_PTR res = 0;
#endif
    LRESULT lRes = SendMessageTimeout((HWND)window, msg, (WPARAM)wParam, (LPARAM)lParam, flags, timeout, &res);
    if (result) *result = (uintptr_t)res;
    return lRes != 0;
}

void* WindowSystemWin32::loadLibrary(const tstring& path) {
    return LoadLibrary(path.c_str());
}

void* WindowSystemWin32::getProcAddress(void* module, const std::string& procName) {
    return (void*)GetProcAddress((HMODULE)module, procName.c_str());
}

bool WindowSystemWin32::freeLibrary(void* module) {
    return FreeLibrary((HMODULE)module) != 0;
}

// Implementation of IWindowSystem interface

yamy::platform::WindowHandle WindowSystemWin32::getForegroundWindow() {
    return fromHWND(GetForegroundWindow());
}

yamy::platform::WindowHandle WindowSystemWin32::windowFromPoint(const yamy::platform::Point& pt) {
    POINT p = { pt.x, pt.y };
    return fromHWND(WindowFromPoint(p));
}

bool WindowSystemWin32::getWindowRect(yamy::platform::WindowHandle hwnd, yamy::platform::Rect* rect) {
    RECT rc;
    if (GetWindowRect(toHWND(hwnd), &rc)) {
        if (rect) {
            rect->left = rc.left;
            rect->top = rc.top;
            rect->right = rc.right;
            rect->bottom = rc.bottom;
        }
        return true;
    }
    return false;
}

std::string WindowSystemWin32::getWindowText(yamy::platform::WindowHandle hwnd) {
    int len = GetWindowTextLength(toHWND(hwnd));
    if (len > 0) {
        std::vector<TCHAR> buf(len + 1);
        GetWindowText(toHWND(hwnd), &buf[0], len + 1);
        #ifdef UNICODE
        return to_string(std::wstring(&buf[0]));
        #else
        return std::string(&buf[0]);
        #endif
    }
    return "";
}

std::string WindowSystemWin32::getWindowClassName(yamy::platform::WindowHandle hwnd) {
    TCHAR className[256];
    if (GetClassName(toHWND(hwnd), className, 256)) {
        #ifdef UNICODE
        return to_string(std::wstring(className));
        #else
        return std::string(className);
        #endif
    }
    return "";
}

bool WindowSystemWin32::bringToForeground(yamy::platform::WindowHandle hwnd) {
    return SetForegroundWindow(toHWND(hwnd)) != 0;
}

bool WindowSystemWin32::moveWindow(yamy::platform::WindowHandle hwnd, const yamy::platform::Rect& rect) {
    return MoveWindow(toHWND(hwnd), rect.left, rect.top, rect.width(), rect.height(), TRUE) != 0;
}

bool WindowSystemWin32::showWindow(yamy::platform::WindowHandle hwnd, int cmdShow) {
    return ShowWindow(toHWND(hwnd), cmdShow) != 0;
}

bool WindowSystemWin32::closeWindow(yamy::platform::WindowHandle hwnd) {
    return PostMessage(toHWND(hwnd), WM_CLOSE, 0, 0) != 0;
}

void WindowSystemWin32::getCursorPos(yamy::platform::Point* pt) {
    POINT p;
    if (GetCursorPos(&p) && pt) {
        pt->x = p.x;
        pt->y = p.y;
    }
}

void WindowSystemWin32::setCursorPos(const yamy::platform::Point& pt) {
    SetCursorPos(pt.x, pt.y);
}

int WindowSystemWin32::getMonitorCount() {
    return GetSystemMetrics(SM_CMONITORS);
}

static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    std::vector<RECT>* monitors = reinterpret_cast<std::vector<RECT>*>(dwData);
    monitors->push_back(*lprcMonitor);
    return TRUE;
}

bool WindowSystemWin32::getMonitorRect(int monitorIndex, yamy::platform::Rect* rect) {
    std::vector<RECT> monitors;
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, (LPARAM)&monitors);

    if (monitorIndex >= 0 && monitorIndex < (int)monitors.size()) {
        if (rect) {
            rect->left = monitors[monitorIndex].left;
            rect->top = monitors[monitorIndex].top;
            rect->right = monitors[monitorIndex].right;
            rect->bottom = monitors[monitorIndex].bottom;
        }
        return true;
    }
    return false;
}

std::string WindowSystemWin32::getClipboardString() {
    tstring ts = getClipboardText();
    #ifdef UNICODE
    return to_string(ts);
    #else
    return ts;
    #endif
}

bool WindowSystemWin32::setClipboardText(const std::string& text) {
    #ifdef UNICODE
    return setClipboardText(to_wstring(text));
    #else
    return setClipboardText(text);
    #endif
}

int WindowSystemWin32::shellExecute(const std::string& operation, const std::string& file, const std::string& parameters, const std::string& directory, int showCmd) {
    #ifdef UNICODE
    return shellExecute(to_wstring(operation), to_wstring(file), to_wstring(parameters), to_wstring(directory), showCmd);
    #else
    return shellExecute(operation, file, parameters, directory, showCmd);
    #endif
}

bool WindowSystemWin32::postMessage(yamy::platform::WindowHandle window, uint32_t message, uintptr_t wParam, intptr_t lParam) {
    return PostMessage(toHWND(window), message, (WPARAM)wParam, (LPARAM)lParam) != 0;
}

bool WindowSystemWin32::getClientRect(yamy::platform::WindowHandle hwnd, yamy::platform::Rect* rect) {
    RECT rc;
    if (GetClientRect(toHWND(hwnd), &rc)) {
        if (rect) {
            rect->left = rc.left;
            rect->top = rc.top;
            rect->right = rc.right;
            rect->bottom = rc.bottom;
        }
        return true;
    }
    return false;
}

bool WindowSystemWin32::getChildWindowRect(yamy::platform::WindowHandle hwnd, yamy::platform::Rect* rect) {
    RECT rc;
    if (!GetWindowRect(toHWND(hwnd), &rc))
        return false;
    POINT p = { rc.left, rc.top };
    HWND phwnd = GetParent(toHWND(hwnd));
    if (!phwnd)
        return false;
    if (!ScreenToClient(phwnd, &p))
        return false;

    if (rect) {
        rect->left = p.x;
        rect->top = p.y;
        p.x = rc.right;
        p.y = rc.bottom;
        ScreenToClient(phwnd, &p);
        rect->right = p.x;
        rect->bottom = p.y;
    }
    return true;
}

namespace yamy::platform {
    IWindowSystem* createWindowSystem() {
        return new WindowSystemWin32();
    }
}
