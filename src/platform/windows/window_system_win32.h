#pragma once
#ifndef _WINDOW_SYSTEM_WIN32_H
#define _WINDOW_SYSTEM_WIN32_H

#include "../../core/window/window_system.h"
#include "../../core/platform/window_system_interface.h"
#include <windows.h>

class WindowSystemWin32 : public WindowSystem, public yamy::platform::IWindowSystem {
public:
    // WindowSystem implementation
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

    // Window Styling and Layering
    bool setWindowZOrder(WindowHandle window, ZOrder order) override;
    bool isWindowTopMost(WindowHandle window) override;
    bool isWindowLayered(WindowHandle window) override;
    bool setWindowLayered(WindowHandle window, bool enable) override;
    bool setLayeredWindowAttributes(WindowHandle window, unsigned long crKey, unsigned char bAlpha, unsigned long dwFlags) override;
    bool redrawWindow(WindowHandle window) override;

    bool enumerateWindows(WindowEnumCallback callback) override;

    int shellExecute(const tstring& operation, const tstring& file, const tstring& parameters, const tstring& directory, int showCmd) override;

    bool disconnectNamedPipe(void* handle) override;
    bool connectNamedPipe(void* handle, void* overlapped) override;
    bool writeFile(void* handle, const void* buffer, unsigned int bytesToWrite, unsigned int* bytesWritten, void* overlapped) override;

    void* openMutex(const tstring& name) override;
    void* openFileMapping(const tstring& name) override;
    void* mapViewOfFile(void* handle) override;
    bool unmapViewOfFile(void* address) override;
    void closeHandle(void* handle) override;
    bool sendMessageTimeout(WindowHandle window, unsigned int msg, uintptr_t wParam, intptr_t lParam, unsigned int flags, unsigned int timeout, uintptr_t* result) override;

    void* loadLibrary(const tstring& path) override;
    void* getProcAddress(void* module, const std::string& procName) override;
    bool freeLibrary(void* module) override;

    // IWindowSystem implementation
    yamy::platform::WindowHandle getForegroundWindow() override;
    yamy::platform::WindowHandle windowFromPoint(const yamy::platform::Point& pt) override;
    // getParent is implicitly covered by WindowSystem::getParent
    yamy::platform::WindowHandle getParent(yamy::platform::WindowHandle hwnd) override;

    bool getWindowRect(yamy::platform::WindowHandle hwnd, yamy::platform::Rect* rect) override;
    bool getClientRect(yamy::platform::WindowHandle hwnd, yamy::platform::Rect* rect) override;
    bool getChildWindowRect(yamy::platform::WindowHandle hwnd, yamy::platform::Rect* rect) override;

    std::string getWindowText(yamy::platform::WindowHandle hwnd) override;
    std::string getWindowClassName(yamy::platform::WindowHandle hwnd) override;

    bool bringToForeground(yamy::platform::WindowHandle hwnd) override;
    bool moveWindow(yamy::platform::WindowHandle hwnd, const yamy::platform::Rect& rect) override;
    bool showWindow(yamy::platform::WindowHandle hwnd, int cmdShow) override;
    bool closeWindow(yamy::platform::WindowHandle hwnd) override;

    void getCursorPos(yamy::platform::Point* pt) override;
    void setCursorPos(const yamy::platform::Point& pt) override;

    int getMonitorCount() override;
    bool getMonitorRect(int monitorIndex, yamy::platform::Rect* rect) override;

    // IWindowSystem extensions
    std::string getClipboardString() override;
    bool setClipboardText(const std::string& text) override;
    int shellExecute(const std::string& operation, const std::string& file, const std::string& parameters, const std::string& directory, int showCmd) override;
    bool postMessage(yamy::platform::WindowHandle window, uint32_t message, uintptr_t wParam, intptr_t lParam) override;

private:
    static HWND toHWND(yamy::platform::WindowHandle handle) {
        return static_cast<HWND>(handle);
    }

    static yamy::platform::WindowHandle fromHWND(HWND hwnd) {
        return static_cast<yamy::platform::WindowHandle>(hwnd);
    }
};

#endif // !_WINDOW_SYSTEM_WIN32_H
