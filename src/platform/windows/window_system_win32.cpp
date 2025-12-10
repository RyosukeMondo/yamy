#include "window_system_win32.h"
#include "utf_conversion.h"
#include "windowstool.h"
#include <vector>
#include <tchar.h>
#include <shellapi.h>

#ifdef UNICODE
#define CF_TTEXT CF_UNICODETEXT
#else
#define CF_TTEXT CF_TEXT
#endif

namespace yamy::platform {

WindowHandle WindowSystemWin32::getParent(WindowHandle window) {
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

std::string WindowSystemWin32::getClipboardText() {
    HGLOBAL hdata;
    const _TCHAR* text = clipboardGetText(&hdata);
    std::string result;
    if (text) {
#ifdef UNICODE
        result = wstring_to_utf8(text);
#else
        result = text;
#endif
    }
    clipboardClose(hdata);
    return result;
}

bool WindowSystemWin32::setClipboardText(const std::string& text) {
    if (!OpenClipboard(nullptr))
        return false;

#ifdef UNICODE
    std::wstring wideText = utf8_to_wstring(text);
    const wchar_t* pText = wideText.c_str();
    size_t size = (wideText.size() + 1) * sizeof(wchar_t);
#else
    const char* pText = text.c_str();
    size_t size = text.size() + 1;
#endif

    HGLOBAL hdataNew = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, size);
    if (!hdataNew) {
        CloseClipboard();
        return false;
    }
    void* dataNew = GlobalLock(hdataNew);
    memcpy(dataNew, pText, size);
    GlobalUnlock(hdataNew);
    
    EmptyClipboard();
    bool result = SetClipboardData(CF_TTEXT, hdataNew) != nullptr;
    
    clipboardClose(nullptr, result ? nullptr : hdataNew); 
    return result;
}

std::string WindowSystemWin32::getClassName(WindowHandle window) {
    HWND hwnd = (HWND)window;
    _TCHAR className[256]; 
    if (GetClassName(hwnd, className, 256)) {
#ifdef UNICODE
        return wstring_to_utf8(className);
#else
        return className;
#endif
    }
    return "";
}

std::string WindowSystemWin32::getTitleName(WindowHandle window) {
    HWND hwnd = (HWND)window;
    int len = GetWindowTextLength(hwnd);
    if (len > 0) {
        std::vector<_TCHAR> buf(len + 1);
        GetWindowText(hwnd, &buf[0], len + 1);
#ifdef UNICODE
        return wstring_to_utf8(&buf[0]);
#else
        return &buf[0];
#endif
    }
    return "";
}

uint32_t WindowSystemWin32::getWindowThreadId(WindowHandle window) {
    return GetWindowThreadProcessId((HWND)window, nullptr);
}

uint32_t WindowSystemWin32::getWindowProcessId(WindowHandle window) {
    DWORD pid = 0;
    GetWindowThreadProcessId((HWND)window, &pid);
    return pid;
}

bool WindowSystemWin32::isConsoleWindow(WindowHandle window) {
#ifdef UNICODE
    return _wcsicmp(utf8_to_wstring(getClassName(window)).c_str(), L"ConsoleWindowClass") == 0;
#else
    return _stricmp(getClassName(window).c_str(), "ConsoleWindowClass") == 0;
#endif
}

bool WindowSystemWin32::setForegroundWindow(WindowHandle window) {
    return SetForegroundWindow((HWND)window) != 0;
}

WindowHandle WindowSystemWin32::getForegroundWindow() {
    return (WindowHandle)GetForegroundWindow();
}

std::string WindowSystemWin32::getWindowText(WindowHandle hwnd) {
    return getTitleName(hwnd);
}

bool WindowSystemWin32::moveWindow(WindowHandle hwnd, const Rect& rect) {
    return MoveWindow((HWND)hwnd, rect.left, rect.top, rect.width(), rect.height(), TRUE) != 0;
}

bool WindowSystemWin32::showWindow(WindowHandle hwnd, int cmdShow) {
    return ShowWindow((HWND)hwnd, cmdShow) != 0;
}

bool WindowSystemWin32::closeWindow(WindowHandle hwnd) {
    return CloseWindow((HWND)hwnd) != 0;
}

int WindowSystemWin32::getMonitorCount() {
    return GetSystemMetrics(SM_CMONITORS);
}

bool WindowSystemWin32::getMonitorRect(int monitorIndex, Rect* rect) {
    // TODO: Use EnumDisplayMonitors to find correct monitor by index
    if (monitorIndex == 0) {
        rect->left = 0;
        rect->top = 0;
        rect->right = GetSystemMetrics(SM_CXSCREEN);
        rect->bottom = GetSystemMetrics(SM_CYSCREEN);
        return true;
    }
    return false;
}

bool WindowSystemWin32::getMonitorWorkArea(int monitorIndex, Rect* rect) {
    if (monitorIndex == 0) {
        RECT rc;
        if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0)) {
            rect->left = rc.left;
            rect->top = rc.top;
            rect->right = rc.right;
            rect->bottom = rc.bottom;
            return true;
        }
    }
    return false;
}

namespace {
    struct MonitorEnumData {
        HMONITOR target;
        int index;
        int foundIndex;
    };

    BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC, LPRECT, LPARAM lParam) {
        MonitorEnumData* d = (MonitorEnumData*)lParam;
        if (hMon == d->target) {
            d->foundIndex = d->index;
            return FALSE;
        }
        d->index++;
        return TRUE;
    }
}

int WindowSystemWin32::getMonitorIndex(WindowHandle window) {
    HMONITOR hMon = MonitorFromWindow((HWND)window, MONITOR_DEFAULTTONEAREST);
    MonitorEnumData data = { hMon, 0, -1 };

    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, (LPARAM)&data);

    return data.foundIndex;
}

void WindowSystemWin32::getCursorPos(Point* outPoint) {
    POINT pt;
    if (GetCursorPos(&pt)) {
        outPoint->x = pt.x;
        outPoint->y = pt.y;
    }
}

void WindowSystemWin32::setCursorPos(const Point& pt) {
    SetCursorPos(pt.x, pt.y);
}

WindowHandle WindowSystemWin32::windowFromPoint(const Point& point) {
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

bool WindowSystemWin32::getWindowRect(WindowHandle window, Rect* outRect) {
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

bool WindowSystemWin32::getClientRect(WindowHandle window, Rect* outRect) {
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

bool WindowSystemWin32::getChildWindowRect(WindowHandle window, Rect* outRect) {
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

bool WindowSystemWin32::getWorkArea(Rect* outRect) {
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

unsigned int WindowSystemWin32::registerWindowMessage(const std::string& name) {
#ifdef UNICODE
    return RegisterWindowMessage(utf8_to_wstring(name).c_str());
#else
    return RegisterWindowMessage(name.c_str());
#endif
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
    IWindowSystem::WindowEnumCallback* callback = (IWindowSystem::WindowEnumCallback*)lParam;
    return (*callback)((WindowHandle)hwnd) ? TRUE : FALSE;
}

bool WindowSystemWin32::enumerateWindows(WindowEnumCallback callback) {
    return EnumWindows(EnumWindowsProc, (LPARAM)&callback) != 0;
}

int WindowSystemWin32::shellExecute(const std::string& operation, const std::string& file, const std::string& parameters, const std::string& directory, int showCmd) {
#ifdef UNICODE
    return (int)(INT_PTR)ShellExecute(
        nullptr,
        operation.empty() ? nullptr : utf8_to_wstring(operation).c_str(),
        file.empty() ? nullptr : utf8_to_wstring(file).c_str(),
        parameters.empty() ? nullptr : utf8_to_wstring(parameters).c_str(),
        directory.empty() ? nullptr : utf8_to_wstring(directory).c_str(),
        showCmd);
#else
    return (int)(INT_PTR)ShellExecute(
        nullptr,
        operation.empty() ? nullptr : operation.c_str(),
        file.empty() ? nullptr : file.c_str(),
        parameters.empty() ? nullptr : parameters.c_str(),
        directory.empty() ? nullptr : directory.c_str(),
        showCmd);
#endif
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

void* WindowSystemWin32::openMutex(const std::string& name) {
#ifdef UNICODE
    return OpenMutex(MUTEX_ALL_ACCESS, FALSE, utf8_to_wstring(name).c_str());
#else
    return OpenMutex(MUTEX_ALL_ACCESS, FALSE, name.c_str());
#endif
}

void* WindowSystemWin32::openFileMapping(const std::string& name) {
#ifdef UNICODE
    return OpenFileMapping(FILE_MAP_READ, FALSE, utf8_to_wstring(name).c_str());
#else
    return OpenFileMapping(FILE_MAP_READ, FALSE, name.c_str());
#endif
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

void* WindowSystemWin32::loadLibrary(const std::string& path) {
#ifdef UNICODE
    return LoadLibrary(utf8_to_wstring(path).c_str());
#else
    return LoadLibrary(path.c_str());
#endif
}

void* WindowSystemWin32::getProcAddress(void* module, const std::string& procName) {
    return (void*)GetProcAddress((HMODULE)module, procName.c_str());
}

bool WindowSystemWin32::freeLibrary(void* module) {
    return FreeLibrary((HMODULE)module) != 0;
}

// Factory implementation
IWindowSystem* createWindowSystem() {
    return new WindowSystemWin32();
}

} // namespace yamy::platform
