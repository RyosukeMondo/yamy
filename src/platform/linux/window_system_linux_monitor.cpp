//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window_system_linux_monitor.cpp
// Track 5: Multi-monitor support using XRandR

#include "window_system_linux_monitor.h"
#include "x11_connection.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <iostream>
#include <vector>
#include <algorithm>

namespace yamy::platform {

WindowSystemLinuxMonitor::WindowSystemLinuxMonitor() {
}

WindowSystemLinuxMonitor::~WindowSystemLinuxMonitor() {
}

// Helper: Get X11 display via centralized connection manager
static Display* getDisplay() {
    return X11Connection::instance().getDisplayOrNull();
}

MonitorHandle WindowSystemLinuxMonitor::getMonitorFromWindow(WindowHandle hwnd) {
    Display* display = getDisplay();
    if (!display || !hwnd) return nullptr;

    Window window = (Window)hwnd;

    // Get window attributes to find its position
    XWindowAttributes attrs;
    if (XGetWindowAttributes(display, window, &attrs) == 0) {
        return getPrimaryMonitor(); // Fallback
    }

    // Use center point of window to determine monitor
    Point center;
    center.x = attrs.x + attrs.width / 2;
    center.y = attrs.y + attrs.height / 2;

    return getMonitorFromPoint(center);
}

MonitorHandle WindowSystemLinuxMonitor::getMonitorFromPoint(const Point& pt) {
    Display* display = getDisplay();
    if (!display) return nullptr;

    Window root = DefaultRootWindow(display);
    XRRScreenResources* res = XRRGetScreenResources(display, root);
    if (!res) return nullptr;

    RRCrtc bestCrtc = 0;

    for (int i = 0; i < res->ncrtc; ++i) {
        XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, res, res->crtcs[i]);
        if (crtcInfo) {
            if (crtcInfo->mode != None) {
                // Check if point is inside this CRTC
                if (pt.x >= crtcInfo->x && pt.x < crtcInfo->x + (int)crtcInfo->width &&
                    pt.y >= crtcInfo->y && pt.y < crtcInfo->y + (int)crtcInfo->height) {

                    bestCrtc = res->crtcs[i];
                    XRRFreeCrtcInfo(crtcInfo);
                    break;
                }
            }
            XRRFreeCrtcInfo(crtcInfo);
        }
    }

    XRRFreeScreenResources(res);

    if (bestCrtc != 0) {
        return (MonitorHandle)(uintptr_t)bestCrtc;
    }

    return getPrimaryMonitor();
}

bool WindowSystemLinuxMonitor::getMonitorRect(MonitorHandle monitor, Rect* rect) {
    if (!rect) return false;

    Display* display = getDisplay();
    if (!display) return false;

    Window root = DefaultRootWindow(display);
    XRRScreenResources* res = XRRGetScreenResources(display, root);
    if (!res) return false;

    RRCrtc crtc = (RRCrtc)(uintptr_t)monitor;
    bool found = false;

    // Verify the CRTC exists and get info
    for (int i = 0; i < res->ncrtc; ++i) {
        if (res->crtcs[i] == crtc) {
            XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, res, crtc);
            if (crtcInfo) {
                rect->left = crtcInfo->x;
                rect->top = crtcInfo->y;
                rect->right = crtcInfo->x + crtcInfo->width;
                rect->bottom = crtcInfo->y + crtcInfo->height;
                XRRFreeCrtcInfo(crtcInfo);
                found = true;
            }
            break;
        }
    }

    // If monitor handle is invalid or not found, try primary
    if (!found) {
        RRCrtc primary = 0;
        RROutput primaryOutput = XRRGetOutputPrimary(display, root);
        if (primaryOutput) {
             XRROutputInfo* outputInfo = XRRGetOutputInfo(display, res, primaryOutput);
             if (outputInfo && outputInfo->crtc) {
                 primary = outputInfo->crtc;
             }
             if (outputInfo) XRRFreeOutputInfo(outputInfo);
        }

        if (primary) {
             XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, res, primary);
             if (crtcInfo) {
                rect->left = crtcInfo->x;
                rect->top = crtcInfo->y;
                rect->right = crtcInfo->x + crtcInfo->width;
                rect->bottom = crtcInfo->y + crtcInfo->height;
                XRRFreeCrtcInfo(crtcInfo);
                found = true;
             }
        }
    }

    XRRFreeScreenResources(res);
    return found;
}

bool WindowSystemLinuxMonitor::getMonitorWorkArea(MonitorHandle monitor, Rect* rect) {
    // Getting the work area (excluding panels) is complex on X11.
    // It usually involves _NET_WORKAREA on the root window.
    // _NET_WORKAREA is an array of 4 CARD32s per desktop: x, y, width, height.

    if (!rect) return false;

    // First get the full monitor rect
    if (!getMonitorRect(monitor, rect)) return false;

    Display* display = getDisplay();
    if (!display) return true; // Return full rect if display fails

    // Get _NET_WORKAREA
    Atom netWorkArea = XInternAtom(display, "_NET_WORKAREA", True);
    if (netWorkArea == None) return true; // Not supported, return full rect

    Atom actualType;
    int actualFormat;
    unsigned long nItems, bytesAfter;
    unsigned char* prop = nullptr;
    Window root = DefaultRootWindow(display);

    if (XGetWindowProperty(display, root, netWorkArea, 0, 1024, False,
                           XA_CARDINAL, &actualType, &actualFormat, &nItems,
                           &bytesAfter, &prop) == Success && prop) {

        // _NET_WORKAREA contains [x, y, width, height] for each desktop.
        // We usually care about the current desktop or the first one.
        // And we need to intersect it with the monitor rect.

        long* workAreas = (long*)prop;
        if (nItems >= 4) {
            Rect workArea;
            workArea.left = workAreas[0];
            workArea.top = workAreas[1];
            workArea.right = workAreas[0] + workAreas[2];
            workArea.bottom = workAreas[1] + workAreas[3];

            // Intersect monitor rect with global work area
            // Note: _NET_WORKAREA is usually the union of all monitors minus struts.
            // On multi-monitor, it might be a single large rectangle or individual ones depending on WM.
            // A common approach is to subtract struts, but that's harder.
            // We will intersect the monitor rect with the work area found.

            rect->left = std::max(rect->left, workArea.left);
            rect->top = std::max(rect->top, workArea.top);
            rect->right = std::min(rect->right, workArea.right);
            rect->bottom = std::min(rect->bottom, workArea.bottom);
        }
        XFree(prop);
    }

    return true;
}

MonitorHandle WindowSystemLinuxMonitor::getPrimaryMonitor() {
    Display* display = getDisplay();
    if (!display) return nullptr;

    Window root = DefaultRootWindow(display);
    XRRScreenResources* res = XRRGetScreenResources(display, root);
    if (!res) return nullptr;

    RROutput primaryOutput = XRRGetOutputPrimary(display, root);
    RRCrtc primaryCrtc = 0;

    if (primaryOutput != None) {
        XRROutputInfo* info = XRRGetOutputInfo(display, res, primaryOutput);
        if (info) {
            primaryCrtc = info->crtc;
            XRRFreeOutputInfo(info);
        }
    }

    // Fallback: use first active CRTC if no primary is set
    if (primaryCrtc == 0) {
        for (int i = 0; i < res->ncrtc; ++i) {
             XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, res, res->crtcs[i]);
             if (crtcInfo) {
                 if (crtcInfo->mode != None) {
                     primaryCrtc = res->crtcs[i];
                     XRRFreeCrtcInfo(crtcInfo);
                     break;
                 }
                 XRRFreeCrtcInfo(crtcInfo);
             }
        }
    }

    XRRFreeScreenResources(res);
    return (MonitorHandle)(uintptr_t)primaryCrtc;
}

} // namespace yamy::platform
