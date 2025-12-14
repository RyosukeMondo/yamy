#include "core/platform/window_system_interface.h"
#include "window_system_linux_queries.h"
#include <iostream>
#include <map>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

namespace yamy::platform {

// X11 error handler (static)
static int windowSystemErrorHandler(Display* display, XErrorEvent* error) {
    char errorText[256];
    XGetErrorText(display, error->error_code, errorText, sizeof(errorText));
    std::cerr << "[WindowSystemLinux] X11 Error: " << errorText
              << " (request code: " << static_cast<int>(error->request_code)
              << ", resource: 0x" << std::hex << error->resourceid << std::dec << ")" << std::endl;
    return 0;  // Don't abort, just log
}

class WindowSystemLinux : public IWindowSystem {
private:
    Display* m_display;
    Window m_rootWindow;
    std::map<std::string, Atom> m_atomCache;

    // Real implementations
    WindowSystemLinuxQueries m_queries;

    // Helper method for atom caching
    Atom getAtom(const char* name) {
        auto it = m_atomCache.find(name);
        if (it != m_atomCache.end()) {
            return it->second;
        }
        if (!m_display) {
            return None;
        }
        Atom atom = XInternAtom(m_display, name, False);
        m_atomCache[name] = atom;
        return atom;
    }

public:
    WindowSystemLinux() : m_display(nullptr), m_rootWindow(None) {
        m_display = XOpenDisplay(nullptr);
        if (!m_display) {
            std::cerr << "[WindowSystemLinux] ERROR: Cannot open X11 display" << std::endl;
            // Continue anyway, methods will handle null display
        } else {
            m_rootWindow = DefaultRootWindow(m_display);
            // Install error handler
            XSetErrorHandler(windowSystemErrorHandler);
            std::cerr << "[WindowSystemLinux] X11 display opened successfully" << std::endl;
        }
    }

    ~WindowSystemLinux() {
        if (m_display) {
            XCloseDisplay(m_display);
            std::cerr << "[WindowSystemLinux] X11 display closed" << std::endl;
        }
    }

    WindowHandle getForegroundWindow() override {
        return m_queries.getForegroundWindow();
    }

    WindowHandle windowFromPoint(const Point& pt) override {
        return m_queries.windowFromPoint(pt);
    }

    bool getWindowRect(WindowHandle hwnd, Rect* rect) override {
        return m_queries.getWindowRect(hwnd, rect);
    }

    std::string getWindowText(WindowHandle hwnd) override {
        return m_queries.getWindowText(hwnd);
    }

    std::string getTitleName(WindowHandle hwnd) override {
        return m_queries.getTitleName(hwnd);
    }

    std::string getClassName(WindowHandle hwnd) override {
        return m_queries.getClassName(hwnd);
    }

    uint32_t getWindowThreadId(WindowHandle hwnd) override {
        return m_queries.getWindowThreadId(hwnd);
    }

    uint32_t getWindowProcessId(WindowHandle hwnd) override {
        return m_queries.getWindowProcessId(hwnd);
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

    WindowHandle getParent(WindowHandle window) override {
        std::cerr << "[STUB] getParent()" << std::endl;
        return nullptr;
    }

    bool isMDIChild(WindowHandle window) override {
        std::cerr << "[STUB] isMDIChild()" << std::endl;
        return false;
    }

    bool isChild(WindowHandle window) override {
        std::cerr << "[STUB] isChild()" << std::endl;
        return false;
    }

    WindowShowCmd getShowCommand(WindowHandle window) override {
        std::cerr << "[STUB] getShowCommand()" << std::endl;
        return WindowShowCmd::Normal; // Default
    }

    bool isConsoleWindow(WindowHandle window) override {
        std::cerr << "[STUB] isConsoleWindow()" << std::endl;
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

    bool getMonitorWorkArea(int monitorIndex, Rect* rect) override {
        std::cerr << "[STUB] getMonitorWorkArea(" << monitorIndex << ")" << std::endl;
        if (rect) {
            *rect = Rect(0, 0, 1920, 1080);
        }
        return false;
    }

    int getMonitorIndex(WindowHandle window) override {
        std::cerr << "[STUB] getMonitorIndex()" << std::endl;
        return 0;
    }

    int getSystemMetrics(SystemMetric metric) override {
        std::cerr << "[STUB] getSystemMetrics()" << std::endl;
        return 0;
    }

    bool getWorkArea(Rect* outRect) override {
        std::cerr << "[STUB] getWorkArea()" << std::endl;
        if (outRect) {
            *outRect = Rect(0, 0, 1920, 1080);
        }
        return false;
    }

    std::string getClipboardText() override {
        std::cerr << "[STUB] getClipboardText()" << std::endl;
        return "";
    }

    bool setClipboardText(const std::string& text) override {
        std::cerr << "[STUB] setClipboardText()" << std::endl;
        return false;
    }

    bool getClientRect(WindowHandle window, Rect* outRect) override {
        std::cerr << "[STUB] getClientRect()" << std::endl;
        if (outRect) {
            *outRect = Rect(0, 0, 800, 600);
        }
        return false;
    }

    bool getChildWindowRect(WindowHandle window, Rect* outRect) override {
        std::cerr << "[STUB] getChildWindowRect()" << std::endl;
        if (outRect) {
            *outRect = Rect(0, 0, 800, 600);
        }
        return false;
    }

    unsigned int mapVirtualKey(unsigned int vkey) override {
        std::cerr << "[STUB] mapVirtualKey(" << vkey << ")" << std::endl;
        return 0;
    }

    bool postMessage(WindowHandle window, unsigned int message, uintptr_t wParam, intptr_t lParam) override {
        std::cerr << "[STUB] postMessage(" << message << ")" << std::endl;
        return false;
    }

    unsigned int registerWindowMessage(const std::string& name) override {
        std::cerr << "[STUB] registerWindowMessage(" << name << ")" << std::endl;
        return 0xC000;
    }

    bool sendMessageTimeout(WindowHandle window, unsigned int msg, uintptr_t wParam, intptr_t lParam, unsigned int flags, unsigned int timeout, uintptr_t* result) override {
        std::cerr << "[STUB] sendMessageTimeout(" << msg << ")" << std::endl;
        if (result) *result = 0;
        return false;
    }

    bool setWindowZOrder(WindowHandle window, ZOrder order) override {
        std::cerr << "[STUB] setWindowZOrder()" << std::endl;
        return false;
    }

    bool isWindowTopMost(WindowHandle window) override {
        std::cerr << "[STUB] isWindowTopMost()" << std::endl;
        return false;
    }

    bool isWindowLayered(WindowHandle window) override {
        std::cerr << "[STUB] isWindowLayered()" << std::endl;
        return false;
    }

    bool setWindowLayered(WindowHandle window, bool enable) override {
        std::cerr << "[STUB] setWindowLayered()" << std::endl;
        return false;
    }

    bool setLayeredWindowAttributes(WindowHandle window, unsigned long crKey, unsigned char bAlpha, unsigned long dwFlags) override {
        std::cerr << "[STUB] setLayeredWindowAttributes()" << std::endl;
        return false;
    }

    bool redrawWindow(WindowHandle window) override {
        std::cerr << "[STUB] redrawWindow()" << std::endl;
        return false;
    }

    bool enumerateWindows(WindowEnumCallback callback) override {
        std::cerr << "[STUB] enumerateWindows()" << std::endl;
        return false;
    }

    int shellExecute(const std::string& operation, const std::string& file, const std::string& parameters, const std::string& directory, int showCmd) override {
        std::cerr << "[STUB] shellExecute(" << operation << ", " << file << ")" << std::endl;
        return 33; // Success > 32
    }

    bool disconnectNamedPipe(void* handle) override {
        std::cerr << "[STUB] disconnectNamedPipe()" << std::endl;
        return false;
    }

    bool connectNamedPipe(void* handle, void* overlapped) override {
        std::cerr << "[STUB] connectNamedPipe()" << std::endl;
        return false;
    }

    bool writeFile(void* handle, const void* buffer, unsigned int bytesToWrite, unsigned int* bytesWritten, void* overlapped) override {
        std::cerr << "[STUB] writeFile()" << std::endl;
        if (bytesWritten) *bytesWritten = 0;
        return false;
    }

    void* openMutex(const std::string& name) override {
        std::cerr << "[STUB] openMutex(" << name << ")" << std::endl;
        return nullptr;
    }

    void* openFileMapping(const std::string& name) override {
        std::cerr << "[STUB] openFileMapping(" << name << ")" << std::endl;
        return nullptr;
    }

    void* mapViewOfFile(void* handle) override {
        std::cerr << "[STUB] mapViewOfFile()" << std::endl;
        return nullptr;
    }

    bool unmapViewOfFile(void* address) override {
        std::cerr << "[STUB] unmapViewOfFile()" << std::endl;
        return false;
    }

    void closeHandle(void* handle) override {
        std::cerr << "[STUB] closeHandle()" << std::endl;
    }

    void* loadLibrary(const std::string& path) override {
        std::cerr << "[STUB] loadLibrary(" << path << ")" << std::endl;
        return nullptr;
    }

    void* getProcAddress(void* module, const std::string& procName) override {
        std::cerr << "[STUB] getProcAddress(" << procName << ")" << std::endl;
        return nullptr;
    }

    bool freeLibrary(void* module) override {
        std::cerr << "[STUB] freeLibrary()" << std::endl;
        return false;
    }

    bool sendCopyData(WindowHandle sender,
                     WindowHandle target,
                     const CopyData& data,
                     uint32_t flags,
                     uint32_t timeout_ms,
                     uintptr_t* result) override {
        std::cerr << "[STUB] sendCopyData()" << std::endl;
        if (result) *result = 0;
        return false;
    }

    WindowHandle getToplevelWindow(WindowHandle hwnd, bool* isMDI) override {
        std::cerr << "[STUB] getToplevelWindow()" << std::endl;
        if (isMDI) *isMDI = false;
        return hwnd;
    }

    bool changeMessageFilter(uint32_t message, uint32_t action) override {
        std::cerr << "[STUB] changeMessageFilter(" << message << ", " << action << ")" << std::endl;
        return true;
    }
};

// Factory implementation
IWindowSystem* createWindowSystem() {
    std::cerr << "[Linux] Creating stub WindowSystem" << std::endl;
    return new WindowSystemLinux();
}

} // namespace yamy::platform
