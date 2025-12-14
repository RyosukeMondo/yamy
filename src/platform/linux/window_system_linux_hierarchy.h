#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window_system_linux_hierarchy.h - Window hierarchy (Track 3)

#include "../../core/platform/types.h"

namespace yamy::platform {

enum class WindowShowCmd; // Forward declaration

/**
 * @class WindowSystemLinuxHierarchy
 * @brief Window hierarchy queries using X11 (Track 3)
 *
 * Provides window hierarchy and state queries using X11/EWMH.
 * Handles parent-child relationships, window states (minimized/maximized),
 * and console window detection.
 *
 * Thread Safety: Safe to use from any thread (X11 connection is synchronized).
 */
class WindowSystemLinuxHierarchy {
public:
    WindowSystemLinuxHierarchy();
    ~WindowSystemLinuxHierarchy();

    /**
     * @brief Get parent window
     *
     * Uses XQueryTree to find the parent window in the X11 hierarchy.
     *
     * @param window Window handle
     * @return Parent window handle, or nullptr if root or invalid
     */
    WindowHandle getParent(WindowHandle window);

    /**
     * @brief Check if window is MDI child
     *
     * On Linux/X11, MDI (Multiple Document Interface) is not a native concept.
     * This method always returns false for compatibility with Windows API.
     *
     * @param window Window handle
     * @return Always false on Linux
     */
    bool isMDIChild(WindowHandle window);

    /**
     * @brief Check if window is a child window
     *
     * Determines if a window has a parent that is not the root window.
     * Uses XQueryTree to check parent relationship.
     *
     * @param window Window handle
     * @return true if window has non-root parent, false otherwise
     */
    bool isChild(WindowHandle window);

    /**
     * @brief Get window show state
     *
     * Queries _NET_WM_STATE to determine if window is minimized, maximized,
     * or in normal state. Checks for _NET_WM_STATE_HIDDEN and
     * _NET_WM_STATE_MAXIMIZED_* atoms.
     *
     * @param window Window handle
     * @return WindowShowCmd enum: Normal, Minimized, or Maximized
     */
    WindowShowCmd getShowCommand(WindowHandle window);

    /**
     * @brief Check if window is a console/terminal
     *
     * Checks WM_CLASS for common terminal emulator classes like "XTerm",
     * "Gnome-terminal", "Konsole", etc.
     *
     * @param window Window handle
     * @return true if window appears to be a terminal, false otherwise
     */
    bool isConsoleWindow(WindowHandle window);

    /**
     * @brief Get top-level window
     *
     * Traverses parent hierarchy to find the top-level window (child of root).
     * Also determines if the window is an MDI child (always false on Linux).
     *
     * @param hwnd Starting window handle
     * @param isMDI Output parameter: set to false on Linux (no MDI support)
     * @return Top-level window handle, or hwnd if already top-level
     */
    WindowHandle getToplevelWindow(WindowHandle hwnd, bool* isMDI);
};

} // namespace yamy::platform
