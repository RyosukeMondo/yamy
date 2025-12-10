#include "core/platform/window_system_interface.h"
#include <iostream>

namespace yamy::platform {

class WindowSystemLinux : public IWindowSystem {
public:
    WindowHandle getForegroundWindow() override {
        std::cerr << "[STUB] getForegroundWindow()" << std::endl;
        return nullptr;
    }

    WindowHandle windowFromPoint(const Point& pt) override {
        std::cerr << "[STUB] windowFromPoint(" << pt.x << ", " << pt.y << ")" << std::endl;
        return nullptr;
    }

    bool getWindowRect(WindowHandle hwnd, Rect* rect) override {
        std::cerr << "[STUB] getWindowRect()" << std::endl;
        if (rect) {
            *rect = Rect(0, 0, 800, 600); // Default rect
        }
        return false;
    }

    std::string getWindowText(WindowHandle hwnd) override {
        std::cerr << "[STUB] getWindowText()" << std::endl;
        return "Stub Window";
    }

    std::string getTitleName(WindowHandle hwnd) override {
        std::cerr << "[STUB] getTitleName()" << std::endl;
        return "Stub Title";
    }

    std::string getClassName(WindowHandle hwnd) override {
        std::cerr << "[STUB] getClassName()" << std::endl;
        return "StubClass";
    }

    bool setForegroundWindow(WindowHandle hwnd) override {
        std::cerr << "[STUB] setForegroundWindow()" << std::endl;
        return false;
    }

    bool moveWindow(WindowHandle hwnd, const Rect& rect) override {
        std::cerr << "[STUB] moveWindow(" << rect.left << "," << rect.top
                  << "," << rect.right << "," << rect.bottom << ")" << std::endl;
        return false;
    }

    bool showWindow(WindowHandle hwnd, int cmdShow) override {
        std::cerr << "[STUB] showWindow()" << std::endl;
        return false;
    }

    bool closeWindow(WindowHandle hwnd) override {
        std::cerr << "[STUB] closeWindow()" << std::endl;
        return false;
    }

    void getCursorPos(Point* pt) override {
        std::cerr << "[STUB] getCursorPos()" << std::endl;
        if (pt) {
            pt->x = 0;
            pt->y = 0;
        }
    }

    void setCursorPos(const Point& pt) override {
        std::cerr << "[STUB] setCursorPos(" << pt.x << ", " << pt.y << ")" << std::endl;
    }

    int getMonitorCount() override {
        std::cerr << "[STUB] getMonitorCount()" << std::endl;
        return 1;
    }

    bool getMonitorRect(int monitorIndex, Rect* rect) override {
        std::cerr << "[STUB] getMonitorRect(" << monitorIndex << ")" << std::endl;
        if (rect) {
            *rect = Rect(0, 0, 1920, 1080);
        }
        return false;
    }

    // New methods added to IWindowSystem
    uint32_t getWindowProcessId(WindowHandle window) override { return 0; }
    uint32_t getWindowThreadId(WindowHandle window) override { return 0; }
    WindowHandle getParent(WindowHandle window) override { return nullptr; }
    bool isChild(WindowHandle window) override { return false; }
    bool isMDIChild(WindowHandle window) override { return false; }
    WindowShowCmd getShowCommand(WindowHandle window) override { return WindowShowCmd::Normal; }
    bool isConsoleWindow(WindowHandle window) override { return false; }
    bool getMonitorWorkArea(int monitorIndex, Rect* rect) override { return false; }
    int getMonitorIndex(WindowHandle window) override { return 0; }
    int getSystemMetrics(SystemMetric metric) override { return 0; }
    bool getWorkArea(Rect* outRect) override { return false; }
    std::string getClipboardText() override { return ""; }
    bool setClipboardText(const std::string& text) override { return false; }
    bool getClientRect(WindowHandle window, Rect* outRect) override { return false; }
    bool getChildWindowRect(WindowHandle window, Rect* outRect) override { return false; }
    unsigned int mapVirtualKey(unsigned int vkey) override { return 0; }
    bool postMessage(WindowHandle window, unsigned int message, uintptr_t wParam, intptr_t lParam) override { return false; }
    unsigned int registerWindowMessage(const std::string& name) override { return 0; }
    bool sendMessageTimeout(WindowHandle window, unsigned int msg, uintptr_t wParam, intptr_t lParam, unsigned int flags, unsigned int timeout, uintptr_t* result) override { return false; }
    bool setWindowZOrder(WindowHandle window, ZOrder order) override { return false; }
    bool isWindowTopMost(WindowHandle window) override { return false; }
    bool isWindowLayered(WindowHandle window) override { return false; }
    bool setWindowLayered(WindowHandle window, bool enable) override { return false; }
    bool setLayeredWindowAttributes(WindowHandle window, unsigned long crKey, unsigned char bAlpha, unsigned long dwFlags) override { return false; }
    bool redrawWindow(WindowHandle window) override { return false; }
    bool enumerateWindows(WindowEnumCallback callback) override { return false; }
    int shellExecute(const std::string& operation, const std::string& file, const std::string& parameters, const std::string& directory, int showCmd) override { return 0; }
    bool disconnectNamedPipe(void* handle) override { return false; }
    bool connectNamedPipe(void* handle, void* overlapped) override { return false; }
    bool writeFile(void* handle, const void* buffer, unsigned int bytesToWrite, unsigned int* bytesWritten, void* overlapped) override { return false; }
    void* openMutex(const std::string& name) override { return nullptr; }
    void* openFileMapping(const std::string& name) override { return nullptr; }
    void* mapViewOfFile(void* handle) override { return nullptr; }
    bool unmapViewOfFile(void* address) override { return false; }
    void closeHandle(void* handle) override {}
    void* loadLibrary(const std::string& path) override { return nullptr; }
    void* getProcAddress(void* module, const std::string& procName) override { return nullptr; }
    bool freeLibrary(void* module) override { return false; }

};

// Factory implementation
IWindowSystem* createWindowSystem() {
    std::cerr << "[Linux] Creating stub WindowSystem" << std::endl;
    return new WindowSystemLinux();
}

} // namespace yamy::platform
