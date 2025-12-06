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
};

#endif // !_WINDOW_SYSTEM_WIN32_H
