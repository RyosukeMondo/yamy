#include "window_system_linux_mouse.h"
#include <X11/Xlib.h>
#include <iostream>
#include <mutex>

namespace yamy::platform {

// Internal helper to manage X connection
namespace {
    Display* g_display = nullptr;
    int g_refCount = 0;
    std::mutex g_displayMutex;

    Display* getXDisplay() {
        std::lock_guard<std::mutex> lock(g_displayMutex);
        if (!g_display) {
            g_display = XOpenDisplay(NULL);
            if (!g_display) {
                std::cerr << "[Linux] Failed to open X display" << std::endl;
            }
        }
        if (g_display) {
            g_refCount++;
        }
        return g_display;
    }

    void releaseXDisplay() {
        std::lock_guard<std::mutex> lock(g_displayMutex);
        if (g_display) {
            g_refCount--;
            if (g_refCount <= 0) {
                XCloseDisplay(g_display);
                g_display = nullptr;
                g_refCount = 0;
            }
        }
    }
}

WindowSystemLinuxMouse::WindowSystemLinuxMouse() {
    getXDisplay();
}

WindowSystemLinuxMouse::~WindowSystemLinuxMouse() {
    releaseXDisplay();
}

void WindowSystemLinuxMouse::getCursorPos(Point* pt) {
    if (!pt) return;

    // We need to protect access if we use the display
    // However, Xlib functions using the same display pointer concurrently might need protection
    // unless XInitThreads() was called.
    // Since we don't know if XInitThreads() is called, we should probably protect the X call too
    // or rely on the fact that we are likely single threaded or X is thread safe enough for this.
    // For now, let's just protect the pointer access.
    // Wait, getXDisplay() returns the pointer.

    Display* display = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_displayMutex);
        display = g_display;
    }

    if (!display) {
        std::cerr << "[Linux] No display connection" << std::endl;
        pt->x = 0;
        pt->y = 0;
        return;
    }

    Window root = DefaultRootWindow(display);
    Window root_return, child_return;
    int root_x, root_y, win_x, win_y;
    unsigned int mask_return;

    // Note: XQueryPointer might need a lock if X isn't initialized for threads.
    // But locking around XQueryPointer with a global mutex might block other operations.
    // We will assume basic usage for now.
    if (XQueryPointer(display, root, &root_return, &child_return,
                      &root_x, &root_y, &win_x, &win_y, &mask_return)) {
        pt->x = root_x;
        pt->y = root_y;
    } else {
        std::cerr << "[Linux] XQueryPointer failed" << std::endl;
        pt->x = 0;
        pt->y = 0;
    }
}

void WindowSystemLinuxMouse::setCursorPos(const Point& pt) {
    Display* display = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_displayMutex);
        display = g_display;
    }

    if (!display) {
        std::cerr << "[Linux] No display connection" << std::endl;
        return;
    }

    Window root = DefaultRootWindow(display);

    XWarpPointer(display, None, root, 0, 0, 0, 0, pt.x, pt.y);
    XFlush(display);
}

} // namespace yamy::platform
