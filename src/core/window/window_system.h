#pragma once
#ifndef _WINDOW_SYSTEM_H
#define _WINDOW_SYSTEM_H

#include "../utils/stringtool.h"
#include <cstdint>
#include <functional>

enum class WindowShowCmd {
    Normal,
    Maximized,
    Minimized,
    Unknown
};

struct WindowPoint {
    long x;
    long y;
};

struct WindowRect {
    long left;
    long top;
    long right;
    long bottom;
};

enum class SystemMetric {
    VirtualScreenWidth,
    VirtualScreenHeight,
    ScreenWidth,
    ScreenHeight
};

enum class ZOrder {
    Top,
    Bottom,
    TopMost,
    NoTopMost
};

class WindowSystem {
public:
    virtual ~WindowSystem() = default;

    typedef void* WindowHandle;

    virtual WindowHandle getParent(WindowHandle window) = 0;
    virtual bool isMDIChild(WindowHandle window) = 0;
    virtual bool isChild(WindowHandle window) = 0;
    virtual WindowShowCmd getShowCommand(WindowHandle window) = 0;
    virtual std::string getClipboardText() = 0;
    virtual bool setClipboardText(const std::string& text) = 0;

    virtual std::string getClassName(WindowHandle window) = 0;
    virtual std::string getTitleName(WindowHandle window) = 0;
    virtual bool isConsoleWindow(WindowHandle window) = 0;
    virtual void setForegroundWindow(WindowHandle window) = 0;

    // New Input/Screen methods
    virtual bool getCursorPos(WindowPoint* outPoint) = 0;
    virtual bool setCursorPos(int x, int y) = 0;
    virtual WindowHandle windowFromPoint(WindowPoint point) = 0;
    virtual int getSystemMetrics(SystemMetric metric) = 0;
    
    virtual unsigned int mapVirtualKey(unsigned int vkey) = 0;
    
    // New methods for function.cpp refactoring
    virtual bool getWindowRect(WindowHandle window, WindowRect* outRect) = 0;
    virtual bool getClientRect(WindowHandle window, WindowRect* outRect) = 0;
    virtual bool getChildWindowRect(WindowHandle window, WindowRect* outRect) = 0;
    virtual bool getWorkArea(WindowRect* outRect) = 0;
    virtual bool postMessage(WindowHandle window, unsigned int message, uintptr_t wParam, intptr_t lParam) = 0;
    virtual unsigned int registerWindowMessage(const std::string& name) = 0;
    
    // Window Styling and Layering
    virtual bool setWindowZOrder(WindowHandle window, ZOrder order) = 0;
    virtual bool isWindowTopMost(WindowHandle window) = 0;
    virtual bool isWindowLayered(WindowHandle window) = 0;
    virtual bool setWindowLayered(WindowHandle window, bool enable) = 0;
    virtual bool setLayeredWindowAttributes(WindowHandle window, unsigned long crKey, unsigned char bAlpha, unsigned long dwFlags) = 0;
    virtual bool redrawWindow(WindowHandle window) = 0;

    // Window Enumeration
    typedef std::function<bool(WindowHandle)> WindowEnumCallback;
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
    virtual bool sendMessageTimeout(WindowHandle window, unsigned int msg, uintptr_t wParam, intptr_t lParam, unsigned int flags, unsigned int timeout, uintptr_t* result) = 0;

    // Dynamic Library
    virtual void* loadLibrary(const std::string& path) = 0;
    virtual void* getProcAddress(void* module, const std::string& procName) = 0;
    virtual bool freeLibrary(void* module) = 0;
};


#endif // !_WINDOW_SYSTEM_H
