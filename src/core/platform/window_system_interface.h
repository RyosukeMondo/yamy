#pragma once
#include "types.h"
#include <string>

namespace yamy::platform {

class IWindowSystem {
public:
    virtual ~IWindowSystem() = default;

    // Window queries
    virtual WindowHandle getForegroundWindow() = 0;
    virtual WindowHandle windowFromPoint(const Point& pt) = 0;
    virtual WindowHandle getParent(WindowHandle hwnd) = 0;

    virtual bool getWindowRect(WindowHandle hwnd, Rect* rect) = 0;
    virtual bool getClientRect(WindowHandle hwnd, Rect* rect) = 0;
    virtual bool getChildWindowRect(WindowHandle hwnd, Rect* rect) = 0;

    virtual std::string getWindowText(WindowHandle hwnd) = 0;
    virtual std::string getWindowClassName(WindowHandle hwnd) = 0;

    // Window manipulation
    virtual bool bringToForeground(WindowHandle hwnd) = 0;
    virtual bool moveWindow(WindowHandle hwnd, const Rect& rect) = 0;
    virtual bool showWindow(WindowHandle hwnd, int cmdShow) = 0;
    virtual bool closeWindow(WindowHandle hwnd) = 0;

    // Cursor
    virtual void getCursorPos(Point* pt) = 0;
    virtual void setCursorPos(const Point& pt) = 0;

    // Monitor info
    virtual int getMonitorCount() = 0;
    virtual bool getMonitorRect(int monitorIndex, Rect* rect) = 0;

    // Clipboard
    virtual std::string getClipboardString() = 0;
    virtual bool setClipboardText(const std::string& text) = 0;

    // Shell / Execution
    virtual int shellExecute(const std::string& operation, const std::string& file, const std::string& parameters, const std::string& directory, int showCmd) = 0;

    // Messaging / IPC (Platform specific abstraction)
    virtual bool postMessage(WindowHandle window, uint32_t message, uintptr_t wParam, intptr_t lParam) = 0;
};

// Factory function (implemented per platform)
IWindowSystem* createWindowSystem();

} // namespace yamy::platform
