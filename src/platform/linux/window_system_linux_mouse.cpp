#include "window_system_linux_mouse.h"
#include "x11_connection.h"
#include <X11/Xlib.h>
#include <iostream>

namespace yamy::platform {

// Helper: Get X11 display via centralized connection manager
static Display* getDisplay() {
    return X11Connection::instance().getDisplayOrNull();
}

WindowSystemLinuxMouse::WindowSystemLinuxMouse() {
    // X11 connection is managed by X11Connection singleton
}

WindowSystemLinuxMouse::~WindowSystemLinuxMouse() {
    // X11 connection lifecycle managed by X11Connection singleton
}

void WindowSystemLinuxMouse::getCursorPos(Point* pt) {
    if (!pt) return;

    Display* display = getDisplay();
    if (!display) {
        pt->x = 0;
        pt->y = 0;
        return;
    }

    Window root = DefaultRootWindow(display);
    Window root_return, child_return;
    int root_x, root_y, win_x, win_y;
    unsigned int mask_return;

    if (XQueryPointer(display, root, &root_return, &child_return,
                      &root_x, &root_y, &win_x, &win_y, &mask_return)) {
        pt->x = root_x;
        pt->y = root_y;
    } else {
        pt->x = 0;
        pt->y = 0;
    }
}

void WindowSystemLinuxMouse::setCursorPos(const Point& pt) {
    Display* display = getDisplay();
    if (!display) return;

    Window root = DefaultRootWindow(display);

    XWarpPointer(display, None, root, 0, 0, 0, 0, pt.x, pt.y);
    XFlush(display);
}

} // namespace yamy::platform
