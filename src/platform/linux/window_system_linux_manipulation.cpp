#include "window_system_linux_manipulation.h"
#include "x11_connection.h"
#include "../../utils/logger.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cstring>

namespace yamy::platform {

// Helper: Get X11 display via centralized connection manager
static Display* getDisplay() {
    return X11Connection::instance().getDisplayOrNull();
}

// Helper: Get atom by name
static Atom getAtom(const char* name) {
    return X11Connection::instance().getAtom(name);
}

WindowSystemLinuxManipulation::WindowSystemLinuxManipulation() {
    LOG_DEBUG("[window] WindowSystemLinuxManipulation initialized");
}

WindowSystemLinuxManipulation::~WindowSystemLinuxManipulation() {}

bool WindowSystemLinuxManipulation::setForegroundWindow(WindowHandle hwnd) {
    if (!hwnd) {
        LOG_DEBUG("[window] setForegroundWindow: null handle");
        return false;
    }

    Display* display = getDisplay();
    if (!display) {
        LOG_DEBUG("[window] setForegroundWindow: no display");
        return false;
    }

    Window window = reinterpret_cast<Window>(hwnd);

    // Raise window
    XRaiseWindow(display, window);

    // Set input focus
    XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);

    // Send _NET_ACTIVE_WINDOW message
    XEvent event;
    memset(&event, 0, sizeof(event));
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = getAtom("_NET_ACTIVE_WINDOW");
    event.xclient.format = 32;
    event.xclient.data.l[0] = 2;  // Source: pager
    event.xclient.data.l[1] = CurrentTime;

    Window root = DefaultRootWindow(display);
    XSendEvent(display, root, False,
               SubstructureNotifyMask | SubstructureRedirectMask,
               &event);

    XFlush(display);
    LOG_INFO("[window] setForegroundWindow(0x{:x}): success", window);
    return true;
}

bool WindowSystemLinuxManipulation::moveWindow(WindowHandle hwnd,
                                               const Rect& rect) {
    if (!hwnd) {
        LOG_DEBUG("[window] moveWindow: null handle");
        return false;
    }

    Display* display = getDisplay();
    if (!display) {
        LOG_DEBUG("[window] moveWindow: no display");
        return false;
    }

    Window window = reinterpret_cast<Window>(hwnd);

    int x = rect.left;
    int y = rect.top;
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    XMoveResizeWindow(display, window, x, y, width, height);
    XFlush(display);

    LOG_INFO("[window] moveWindow(0x{:x}): pos=({}, {}) size={}x{}", window, x, y, width, height);
    return true;
}

bool WindowSystemLinuxManipulation::showWindow(WindowHandle hwnd,
                                               int cmdShow) {
    if (!hwnd) {
        LOG_DEBUG("[window] showWindow: null handle");
        return false;
    }

    Display* display = getDisplay();
    if (!display) {
        LOG_DEBUG("[window] showWindow: no display");
        return false;
    }

    Window window = reinterpret_cast<Window>(hwnd);

    // cmdShow values (Windows compatible):
    // 0 = SW_HIDE, 1 = SW_SHOWNORMAL, 3 = SW_MAXIMIZE, 6 = SW_MINIMIZE
    const char* cmdName = "unknown";

    switch (cmdShow) {
        case 0: // Hide
            XUnmapWindow(display, window);
            cmdName = "hide";
            break;

        case 6: // Minimize
            XIconifyWindow(display, window, DefaultScreen(display));
            cmdName = "minimize";
            break;

        case 1: // Show normal
            cmdName = "show";
            XMapWindow(display, window);
            XRaiseWindow(display, window);
            break;

        case 3: // Maximize (TODO: set _NET_WM_STATE)
            cmdName = "maximize";
            XMapWindow(display, window);
            XRaiseWindow(display, window);
            break;

        default:
            cmdName = "default";
            XMapWindow(display, window);
            XRaiseWindow(display, window);
            break;
    }

    XFlush(display);
    LOG_INFO("[window] showWindow(0x{:x}): cmd={} ({})", window, cmdShow, cmdName);
    return true;
}

bool WindowSystemLinuxManipulation::closeWindow(WindowHandle hwnd) {
    if (!hwnd) {
        LOG_DEBUG("[window] closeWindow: null handle");
        return false;
    }

    Display* display = getDisplay();
    if (!display) {
        LOG_DEBUG("[window] closeWindow: no display");
        return false;
    }

    Window window = reinterpret_cast<Window>(hwnd);

    // Send WM_DELETE_WINDOW protocol message
    Atom wmProtocols = getAtom("WM_PROTOCOLS");
    Atom wmDeleteWindow = getAtom("WM_DELETE_WINDOW");

    XEvent event;
    memset(&event, 0, sizeof(event));
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = wmProtocols;
    event.xclient.format = 32;
    event.xclient.data.l[0] = wmDeleteWindow;
    event.xclient.data.l[1] = CurrentTime;

    XSendEvent(display, window, False, NoEventMask, &event);
    XFlush(display);

    LOG_INFO("[window] closeWindow(0x{:x}): WM_DELETE_WINDOW sent", window);
    return true;
}

uint32_t WindowSystemLinuxManipulation::registerWindowMessage(
    const std::string& name) {
    // In X11, messages are atoms
    Display* display = getDisplay();
    if (!display) return 0;

    Atom atom = XInternAtom(display, name.c_str(), False);
    return static_cast<uint32_t>(atom);
}

bool WindowSystemLinuxManipulation::sendMessageTimeout(
    WindowHandle hwnd, uint32_t msg,
    uintptr_t wParam, intptr_t lParam,
    uint32_t flags, uint32_t timeout,
    uintptr_t* result) {

    if (!hwnd) return false;

    Display* display = getDisplay();
    if (!display) return false;

    Window window = reinterpret_cast<Window>(hwnd);

    // Send custom client message
    XEvent event;
    memset(&event, 0, sizeof(event));
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = static_cast<Atom>(msg);
    event.xclient.format = 32;
    event.xclient.data.l[0] = wParam;
    event.xclient.data.l[1] = lParam;

    Status status = XSendEvent(display, window, False,
                              NoEventMask, &event);
    XFlush(display);

    if (result) *result = 0;
    return (status != 0);
}

} // namespace yamy::platform
