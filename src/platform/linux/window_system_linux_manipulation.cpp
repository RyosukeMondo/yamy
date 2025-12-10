#include "window_system_linux_manipulation.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cstring>

namespace yamy::platform {

static Display* getDisplay() {
    static Display* display = XOpenDisplay(nullptr);
    return display;
}

static Atom getAtom(const char* name) {
    return XInternAtom(getDisplay(), name, False);
}

WindowSystemLinuxManipulation::WindowSystemLinuxManipulation() {}
WindowSystemLinuxManipulation::~WindowSystemLinuxManipulation() {}

bool WindowSystemLinuxManipulation::setForegroundWindow(WindowHandle hwnd) {
    if (!hwnd) return false;

    Display* display = getDisplay();
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
    return true;
}

bool WindowSystemLinuxManipulation::moveWindow(WindowHandle hwnd,
                                               const Rect& rect) {
    if (!hwnd) return false;

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    int x = rect.left;
    int y = rect.top;
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    XMoveResizeWindow(display, window, x, y, width, height);
    XFlush(display);

    return true;
}

bool WindowSystemLinuxManipulation::showWindow(WindowHandle hwnd,
                                               int cmdShow) {
    if (!hwnd) return false;

    Display* display = getDisplay();
    Window window = reinterpret_cast<Window>(hwnd);

    // cmdShow values (Windows compatible):
    // 0 = SW_HIDE, 1 = SW_SHOWNORMAL, 3 = SW_MAXIMIZE, 6 = SW_MINIMIZE

    switch (cmdShow) {
        case 0: // Hide
            XUnmapWindow(display, window);
            break;

        case 6: // Minimize
            XIconifyWindow(display, window, DefaultScreen(display));
            break;

        case 1: // Show normal
        case 3: // Maximize (TODO: set _NET_WM_STATE)
        default:
            XMapWindow(display, window);
            XRaiseWindow(display, window);
            break;
    }

    XFlush(display);
    return true;
}

bool WindowSystemLinuxManipulation::closeWindow(WindowHandle hwnd) {
    if (!hwnd) return false;

    Display* display = getDisplay();
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

    return true;
}

uint32_t WindowSystemLinuxManipulation::registerWindowMessage(
    const std::string& name) {
    // In X11, messages are atoms
    Atom atom = XInternAtom(getDisplay(), name.c_str(), False);
    return static_cast<uint32_t>(atom);
}

bool WindowSystemLinuxManipulation::sendMessageTimeout(
    WindowHandle hwnd, uint32_t msg,
    uintptr_t wParam, intptr_t lParam,
    uint32_t flags, uint32_t timeout,
    uintptr_t* result) {

    if (!hwnd) return false;

    Display* display = getDisplay();
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
