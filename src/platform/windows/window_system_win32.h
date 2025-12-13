#pragma once
#ifndef _WINDOW_SYSTEM_WIN32_H
#define _WINDOW_SYSTEM_WIN32_H

#include "../../core/platform/window_system_interface.h"
#include <windows.h>

namespace yamy::platform {

class WindowSystemWin32 : public IWindowSystem {
public:
    WindowHandle getParent(WindowHandle window) override;
    bool isMDIChild(WindowHandle window) override;
    bool isChild(WindowHandle window) override;
    WindowShowCmd getShowCommand(WindowHandle window) override;
    std::string getClipboardText() override;
    bool setClipboardText(const std::string& text) override;
    std::string getClassName(WindowHandle window) override;
    std::string getTitleName(WindowHandle window) override;
    uint32_t getWindowThreadId(WindowHandle window) override;
    uint32_t getWindowProcessId(WindowHandle window) override;
    bool isConsoleWindow(WindowHandle window) override;
    bool setForegroundWindow(WindowHandle window) override;

    // From original IWindowSystem interface
    WindowHandle getForegroundWindow() override;
    std::string getWindowText(WindowHandle window) override;
    bool moveWindow(WindowHandle hwnd, const Rect& rect) override;
    bool showWindow(WindowHandle hwnd, int cmdShow) override;
    bool closeWindow(WindowHandle hwnd) override;
    void getCursorPos(Point* pt) override;
    void setCursorPos(const Point& pt) override;
    int getMonitorCount() override;
    bool getMonitorRect(int monitorIndex, Rect* rect) override;
    bool getMonitorWorkArea(int monitorIndex, Rect* rect) override;
    int getMonitorIndex(WindowHandle window) override;

    WindowHandle windowFromPoint(const Point& point) override;
    int getSystemMetrics(SystemMetric metric) override;
    unsigned int mapVirtualKey(unsigned int vkey) override;

    bool getWindowRect(WindowHandle window, Rect* outRect) override;
    bool getClientRect(WindowHandle window, Rect* outRect) override;
    bool getChildWindowRect(WindowHandle window, Rect* outRect) override;
    bool getWorkArea(Rect* outRect) override;
    bool postMessage(WindowHandle window, unsigned int message, uintptr_t wParam, intptr_t lParam) override;
    unsigned int registerWindowMessage(const std::string& name) override;

    // Window Styling and Layering
    bool setWindowZOrder(WindowHandle window, ZOrder order) override;
    bool isWindowTopMost(WindowHandle window) override;
    bool isWindowLayered(WindowHandle window) override;
    bool setWindowLayered(WindowHandle window, bool enable) override;
    bool setLayeredWindowAttributes(WindowHandle window, unsigned long crKey, unsigned char bAlpha, unsigned long dwFlags) override;
    bool redrawWindow(WindowHandle window) override;

    bool enumerateWindows(WindowEnumCallback callback) override;

    int shellExecute(const std::string& operation, const std::string& file, const std::string& parameters, const std::string& directory, int showCmd) override;

    bool disconnectNamedPipe(void* handle) override;
    bool connectNamedPipe(void* handle, void* overlapped) override;
    bool writeFile(void* handle, const void* buffer, unsigned int bytesToWrite, unsigned int* bytesWritten, void* overlapped) override;

    void* openMutex(const std::string& name) override;
    void* openFileMapping(const std::string& name) override;
    void* mapViewOfFile(void* handle) override;
    bool unmapViewOfFile(void* address) override;
    void closeHandle(void* handle) override;
    bool sendMessageTimeout(WindowHandle window, unsigned int msg, uintptr_t wParam, intptr_t lParam, unsigned int flags, unsigned int timeout, uintptr_t* result) override;

    void* loadLibrary(const std::string& path) override;
    void* getProcAddress(void* module, const std::string& procName) override;
    bool freeLibrary(void* module) override;

    bool sendCopyData(WindowHandle sender,
                     WindowHandle target,
                     const CopyData& data,
                     uint32_t flags,
                     uint32_t timeout_ms,
                     uintptr_t* result) override;

    WindowHandle getToplevelWindow(WindowHandle hwnd, bool* isMDI) override;
    bool changeMessageFilter(uint32_t message, uint32_t action) override;
};

} // namespace yamy::platform

#endif // !_WINDOW_SYSTEM_WIN32_H
