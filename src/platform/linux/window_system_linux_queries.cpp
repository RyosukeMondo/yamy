#include "window_system_linux_queries.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cstring>

namespace yamy::platform {

// Helper: Get X11 display (create once, reuse)
static Display* getDisplay() {
    static Display* display = XOpenDisplay(nullptr);
    return display;
}

// Helper: Get atom by name
static Atom getAtom(const char* name) {
    Display* display = getDisplay();
    return XInternAtom(display, name, False);
}

// Helper: Get window property
static bool getWindowProperty(WindowHandle hwnd, Atom property,
                             Atom type, unsigned char** data,
                             unsigned long* items) {
    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    Atom actual_type;
    int actual_format;
    unsigned long bytes_after;

    int status = XGetWindowProperty(display, window, property, 0, (~0L),
                                    False, type, &actual_type, &actual_format,
                                    items, &bytes_after, data);

    return (status == Success && *items > 0);
}

WindowSystemLinuxQueries::WindowSystemLinuxQueries() {
    // Initialize X11 connection
    getDisplay();
}

WindowSystemLinuxQueries::~WindowSystemLinuxQueries() {
    // Don't close display (it's static)
}

WindowHandle WindowSystemLinuxQueries::getForegroundWindow() {
    Display* display = getDisplay();
    if (!display) return nullptr;

    // Method 1: Try _NET_ACTIVE_WINDOW
    Atom netActiveWindow = getAtom("_NET_ACTIVE_WINDOW");
    Window root = DefaultRootWindow(display);

    unsigned char* data = nullptr;
    unsigned long items = 0;

    if (getWindowProperty(reinterpret_cast<WindowHandle>(root),
                         netActiveWindow, XA_WINDOW,
                         &data, &items)) {
        Window* active = reinterpret_cast<Window*>(data);
        Window result = *active;
        XFree(data);
        return reinterpret_cast<WindowHandle>(result);
    }

    // Method 2: Fallback to XGetInputFocus
    Window focus;
    int revert_to;
    XGetInputFocus(display, &focus, &revert_to);

    return reinterpret_cast<WindowHandle>(focus);
}

WindowHandle WindowSystemLinuxQueries::windowFromPoint(const Point& pt) {
    Display* display = getDisplay();
    if (!display) return nullptr;

    Window root = DefaultRootWindow(display);
    Window child = root;
    int root_x = pt.x, root_y = pt.y;
    int win_x, win_y;
    unsigned int mask;

    // Query pointer to find window at position
    if (XQueryPointer(display, root, &root, &child,
                     &root_x, &root_y, &win_x, &win_y, &mask)) {
        // Traverse to find deepest window at position
        while (child != None) {
            Window temp_child;
            XTranslateCoordinates(display, root, child,
                                 pt.x, pt.y, &win_x, &win_y, &temp_child);
            if (temp_child == None) break;
            child = temp_child;
        }

        return reinterpret_cast<WindowHandle>(child);
    }

    return nullptr;
}

std::string WindowSystemLinuxQueries::getWindowText(WindowHandle hwnd) {
    if (!hwnd) return "";

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    // Try UTF-8 name first (_NET_WM_NAME)
    Atom netWmName = getAtom("_NET_WM_NAME");
    Atom utf8String = getAtom("UTF8_STRING");
    unsigned char* data = nullptr;
    unsigned long items = 0;

    if (getWindowProperty(hwnd, netWmName, utf8String, &data, &items)) {
        std::string result(reinterpret_cast<char*>(data));
        XFree(data);
        return result;
    }

    // Fallback to WM_NAME
    char* name = nullptr;
    if (XFetchName(display, window, &name)) {
        std::string result(name);
        XFree(name);
        return result;
    }

    return "";
}

std::string WindowSystemLinuxQueries::getTitleName(WindowHandle hwnd) {
    return getWindowText(hwnd);  // Same as window text
}

std::string WindowSystemLinuxQueries::getClassName(WindowHandle hwnd) {
    if (!hwnd) return "";

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    XClassHint class_hint;
    if (XGetClassHint(display, window, &class_hint)) {
        std::string result;
        if (class_hint.res_class) {
            result = class_hint.res_class;
        } else if (class_hint.res_name) {
            result = class_hint.res_name;
        }

        if (class_hint.res_name) XFree(class_hint.res_name);
        if (class_hint.res_class) XFree(class_hint.res_class);

        return result;
    }

    return "";
}

uint32_t WindowSystemLinuxQueries::getWindowThreadId(WindowHandle hwnd) {
    // Linux doesn't have per-window thread IDs
    // Return process ID instead (same as getWindowProcessId)
    return getWindowProcessId(hwnd);
}

uint32_t WindowSystemLinuxQueries::getWindowProcessId(WindowHandle hwnd) {
    if (!hwnd) return 0;

    Atom netWmPid = getAtom("_NET_WM_PID");
    unsigned char* data = nullptr;
    unsigned long items = 0;

    if (getWindowProperty(hwnd, netWmPid, XA_CARDINAL, &data, &items)) {
        uint32_t pid = *reinterpret_cast<uint32_t*>(data);
        XFree(data);
        return pid;
    }

    return 0;
}

bool WindowSystemLinuxQueries::getWindowRect(WindowHandle hwnd, Rect* rect) {
    if (!hwnd || !rect) return false;

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    XWindowAttributes attrs;
    if (!XGetWindowAttributes(display, window, &attrs)) {
        return false;
    }

    // Translate to screen coordinates
    Window child;
    int x, y;
    XTranslateCoordinates(display, window, attrs.root,
                         0, 0, &x, &y, &child);

    rect->left = x;
    rect->top = y;
    rect->right = x + attrs.width;
    rect->bottom = y + attrs.height;

    return true;
}

} // namespace yamy::platform
