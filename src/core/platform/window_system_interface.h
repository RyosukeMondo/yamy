#pragma once
#include "types.h"
#include <string>
#include <functional>

namespace yamy::platform {

class IWindowSystem {
public:
    virtual ~IWindowSystem() = default;

    // Window queries
    virtual WindowHandle getForegroundWindow() = 0;
    virtual WindowHandle windowFromPoint(const Point& pt) = 0;
    virtual bool getWindowRect(WindowHandle hwnd, Rect* rect) = 0;
    virtual std::string getWindowText(WindowHandle hwnd) = 0;
    virtual std::string getClassName(WindowHandle hwnd) = 0;
    virtual std::string getTitleName(WindowHandle hwnd) = 0;
    virtual uint32_t getWindowThreadId(WindowHandle hwnd) = 0;
    virtual uint32_t getWindowProcessId(WindowHandle hwnd) = 0;

    // Window manipulation
    virtual bool setForegroundWindow(WindowHandle hwnd) = 0;
    virtual bool moveWindow(WindowHandle hwnd, const Rect& rect) = 0;
    virtual bool showWindow(WindowHandle hwnd, int cmdShow) = 0;
    virtual bool closeWindow(WindowHandle hwnd) = 0;
    virtual WindowHandle getParent(WindowHandle window) = 0;
    virtual bool isMDIChild(WindowHandle window) = 0;
    virtual bool isChild(WindowHandle window) = 0;
    virtual WindowShowCmd getShowCommand(WindowHandle window) = 0;
    virtual bool isConsoleWindow(WindowHandle window) = 0;

    // Cursor
    virtual void getCursorPos(Point* pt) = 0;
    virtual void setCursorPos(const Point& pt) = 0;

    // Monitor info
    virtual int getMonitorCount() = 0;
    virtual bool getMonitorRect(int monitorIndex, Rect* rect) = 0;
    virtual bool getMonitorWorkArea(int monitorIndex, Rect* rect) = 0;
    virtual int getMonitorIndex(WindowHandle window) = 0;
    virtual int getSystemMetrics(SystemMetric metric) = 0;
    virtual bool getWorkArea(Rect* outRect) = 0;

    // Clipboard
    virtual std::string getClipboardText() = 0;
    virtual bool setClipboardText(const std::string& text) = 0;

    // Rects
    virtual bool getClientRect(WindowHandle window, Rect* outRect) = 0;
    virtual bool getChildWindowRect(WindowHandle window, Rect* outRect) = 0;

    // Keyboard/Input mapping
    virtual unsigned int mapVirtualKey(unsigned int vkey) = 0;

    // Messaging
    virtual bool postMessage(WindowHandle window, unsigned int message, uintptr_t wParam, intptr_t lParam) = 0;
    virtual unsigned int registerWindowMessage(const std::string& name) = 0;
    virtual bool sendMessageTimeout(WindowHandle window, unsigned int msg, uintptr_t wParam, intptr_t lParam, unsigned int flags, unsigned int timeout, uintptr_t* result) = 0;

    // Styling/Layering
    virtual bool setWindowZOrder(WindowHandle window, ZOrder order) = 0;
    virtual bool isWindowTopMost(WindowHandle window) = 0;
    virtual bool isWindowLayered(WindowHandle window) = 0;
    virtual bool setWindowLayered(WindowHandle window, bool enable) = 0;
    virtual bool setLayeredWindowAttributes(WindowHandle window, unsigned long crKey, unsigned char bAlpha, unsigned long dwFlags) = 0;
    virtual bool redrawWindow(WindowHandle window) = 0;

    // Enumeration
    using WindowEnumCallback = std::function<bool(WindowHandle)>;
    virtual bool enumerateWindows(WindowEnumCallback callback) = 0;

    // Shell
    virtual int shellExecute(const std::string& operation, const std::string& file, const std::string& parameters, const std::string& directory, int showCmd) = 0;

    // IPC / Pipe wrappers
    virtual bool disconnectNamedPipe(void* handle) = 0;
    virtual bool connectNamedPipe(void* handle, void* overlapped) = 0;
    virtual bool writeFile(void* handle, const void* buffer, unsigned int bytesToWrite, unsigned int* bytesWritten, void* overlapped) = 0;

    // IPC (Mutex/Shared Memory)
    virtual void* openMutex(const std::string& name) = 0;
    virtual void* openFileMapping(const std::string& name) = 0;
    virtual void* mapViewOfFile(void* handle) = 0;
    virtual bool unmapViewOfFile(void* address) = 0;
    virtual void closeHandle(void* handle) = 0;

    // Dynamic Library
    virtual void* loadLibrary(const std::string& path) = 0;
    virtual void* getProcAddress(void* module, const std::string& procName) = 0;
    virtual bool freeLibrary(void* module) = 0;
};

// Factory function
IWindowSystem* createWindowSystem();

} // namespace yamy::platform
