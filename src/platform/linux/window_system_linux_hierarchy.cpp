//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// src/platform/linux/window_system_linux_hierarchy.cpp
// Track 3: X11 Window Hierarchy
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "window_system_linux_hierarchy.h"
#include "x11_connection.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

// Use standard headers for Linux environment
#include <unistd.h>

namespace yamy::platform {

// Helper: Get X11 display via centralized connection manager
static Display* getDisplay() {
    return X11Connection::instance().getDisplayOrNull();
}

// Helper: Get atom by name
static Atom getAtom(const char* name) {
    return X11Connection::instance().getAtom(name);
}

WindowSystemLinuxHierarchy::WindowSystemLinuxHierarchy() {
    // X11 connection is managed by X11Connection singleton
}

WindowSystemLinuxHierarchy::~WindowSystemLinuxHierarchy() {
    // X11 connection lifecycle managed by X11Connection singleton
}

WindowHandle WindowSystemLinuxHierarchy::getParent(WindowHandle window) {
    Display* dpy = getDisplay();
    if (!dpy || !window) return nullptr;

    Window w = reinterpret_cast<Window>(window);
    Window root_return, parent_return;
    Window* children_return = nullptr;
    unsigned int nchildren_return = 0;

    Status status = XQueryTree(dpy, w, &root_return, &parent_return, &children_return, &nchildren_return);

    if (children_return) {
        XFree(children_return);
    }

    if (status && parent_return != 0) {
        return reinterpret_cast<WindowHandle>(parent_return);
    }

    return nullptr;
}

bool WindowSystemLinuxHierarchy::isMDIChild(WindowHandle window) {
    // Linux doesn't really have MDI in the Windows sense.
    // However, we check _NET_WM_WINDOW_TYPE as requested.
    Display* dpy = getDisplay();
    if (!dpy || !window) return false;

    Window w = reinterpret_cast<Window>(window);
    Atom typeAtom = getAtom("_NET_WM_WINDOW_TYPE");
    Atom actualType;
    int actualFormat;
    unsigned long nItems, bytesAfter;
    unsigned char* prop = nullptr;

    if (XGetWindowProperty(dpy, w, typeAtom, 0, 1024, False, XA_ATOM,
                          &actualType, &actualFormat, &nItems, &bytesAfter, &prop) == Success) {
        if (prop) {
            Atom* atoms = reinterpret_cast<Atom*>(prop);
            for (unsigned long i = 0; i < nItems; ++i) {
                // If we were checking for specific types, we'd do it here.
                // But for "isMDIChild", we generally return false on Linux as per prompt.
                // Just ensuring we accessed the property.
                (void)atoms[i];
            }
            XFree(prop);
        }
    }

    return false; // Always false on Linux as per instructions
}

bool WindowSystemLinuxHierarchy::isChild(WindowHandle window) {
    Display* dpy = getDisplay();
    if (!dpy || !window) return false;

    Window w = reinterpret_cast<Window>(window);
    Window root_return, parent_return;
    Window* children_return = nullptr;
    unsigned int nchildren_return = 0;

    Status status = XQueryTree(dpy, w, &root_return, &parent_return, &children_return, &nchildren_return);

    if (children_return) {
        XFree(children_return);
    }

    // A window is a "child" if it has a parent and that parent is not the root window?
    // Or just if it has a parent?
    // In X11, top-level windows have RootWindow as parent.
    // So if parent == RootWindow, it is NOT a child (it is a top-level).
    // If parent != RootWindow, it is a child.

    if (status && parent_return != 0) {
        return parent_return != root_return;
    }

    return false;
}

WindowShowCmd WindowSystemLinuxHierarchy::getShowCommand(WindowHandle window) {
    Display* dpy = getDisplay();
    if (!dpy || !window) return WindowShowCmd::Normal;

    Window w = reinterpret_cast<Window>(window);
    Atom stateAtom = getAtom("_NET_WM_STATE");
    Atom hiddenAtom = getAtom("_NET_WM_STATE_HIDDEN");
    Atom maxVertAtom = getAtom("_NET_WM_STATE_MAXIMIZED_VERT");
    Atom maxHorzAtom = getAtom("_NET_WM_STATE_MAXIMIZED_HORZ");

    Atom actualType;
    int actualFormat;
    unsigned long nItems, bytesAfter;
    unsigned char* prop = nullptr;

    bool isHidden = false;
    bool isMaxVert = false;
    bool isMaxHorz = false;

    if (XGetWindowProperty(dpy, w, stateAtom, 0, 1024, False, XA_ATOM,
                          &actualType, &actualFormat, &nItems, &bytesAfter, &prop) == Success) {
        if (prop) {
            Atom* atoms = reinterpret_cast<Atom*>(prop);
            for (unsigned long i = 0; i < nItems; ++i) {
                if (atoms[i] == hiddenAtom) isHidden = true;
                if (atoms[i] == maxVertAtom) isMaxVert = true;
                if (atoms[i] == maxHorzAtom) isMaxHorz = true;
            }
            XFree(prop);
        }
    }

    if (isHidden) return WindowShowCmd::Minimized;
    if (isMaxVert && isMaxHorz) return WindowShowCmd::Maximized;

    return WindowShowCmd::Normal;
}

bool WindowSystemLinuxHierarchy::isConsoleWindow(WindowHandle window) {
    Display* dpy = getDisplay();
    if (!dpy || !window) return false;

    Window w = reinterpret_cast<Window>(window);
    XClassHint classHint;
    bool isConsole = false;

    if (XGetClassHint(dpy, w, &classHint)) {
        if (classHint.res_class) {
            std::string cls = classHint.res_class;
            // Common terminal emulators
            if (cls == "XTerm" || cls == "URxvt" || cls == "Gnome-terminal" ||
                cls == "Konsole" || cls == "Terminator" || cls == "Alacritty" ||
                cls == "kitty") {
                isConsole = true;
            }
        }
        if (!isConsole && classHint.res_name) {
             std::string name = classHint.res_name;
             if (name.find("term") != std::string::npos) {
                 isConsole = true;
             }
        }

        if (classHint.res_name) XFree(classHint.res_name);
        if (classHint.res_class) XFree(classHint.res_class);
    }

    return isConsole;
}

WindowHandle WindowSystemLinuxHierarchy::getToplevelWindow(WindowHandle hwnd, bool* isMDI) {
    Display* dpy = getDisplay();
    if (isMDI) *isMDI = false; // Always false on Linux
    if (!dpy || !hwnd) return hwnd;

    Window current = reinterpret_cast<Window>(hwnd);
    Window root = DefaultRootWindow(dpy);

    // Safety break to prevent infinite loops
    int maxDepth = 50;

    while (maxDepth-- > 0) {
        Window root_ret, parent_ret;
        Window* children = nullptr;
        unsigned int nchildren = 0;

        Status s = XQueryTree(dpy, current, &root_ret, &parent_ret, &children, &nchildren);
        if (children) XFree(children);

        if (!s || parent_ret == 0 || parent_ret == root_ret || parent_ret == root) {
            // Found top level (parent is root or 0)
            break;
        }
        current = parent_ret;
    }

    return reinterpret_cast<WindowHandle>(current);
}

} // namespace yamy::platform
