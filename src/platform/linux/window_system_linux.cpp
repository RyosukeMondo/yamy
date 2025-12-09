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
};

// Factory implementation
IWindowSystem* createWindowSystem() {
    std::cerr << "[Linux] Creating stub WindowSystem" << std::endl;
    return new WindowSystemLinux();
}

} // namespace yamy::platform
