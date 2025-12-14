#pragma once
#include <cstdint>

/**
 * @file types.h
 * @brief Platform-agnostic type definitions for YAMY core.
 *
 * Defines platform-neutral types for window management, geometry, input events,
 * and system resources. These types abstract platform-specific representations
 * (e.g., HWND on Windows, Window on X11).
 */

namespace yamy::platform {

/**
 * @brief Platform-agnostic window handle.
 *
 * Represents a native window handle without exposing platform-specific types.
 * On Windows: HWND, On Linux: Window (X11) or similar.
 */
using WindowHandle = void*;

/**
 * @brief Platform-agnostic 2D point.
 *
 * Represents screen coordinates or offsets.
 *
 * @code
 * Point cursor(100, 200);
 * Point offset = Point(cursor.x + 10, cursor.y + 10);
 * @endcode
 */
struct Point {
    int32_t x; ///< Horizontal coordinate
    int32_t y; ///< Vertical coordinate

    Point() : x(0), y(0) {}
    Point(int32_t x, int32_t y) : x(x), y(y) {}
};

/**
 * @brief Platform-agnostic rectangle.
 *
 * Represents a rectangular region using left, top, right, bottom coordinates.
 * Provides utility methods for size calculation and containment testing.
 *
 * @code
 * Rect window(0, 0, 800, 600);
 * Rect inner(10, 10, 790, 590);
 * if (inner.isContainedIn(window)) {
 *     // inner is fully within window bounds
 * }
 * @endcode
 */
struct Rect {
    int32_t left;   ///< Left edge coordinate
    int32_t top;    ///< Top edge coordinate
    int32_t right;  ///< Right edge coordinate
    int32_t bottom; ///< Bottom edge coordinate

    Rect() : left(0), top(0), right(0), bottom(0) {}
    Rect(int32_t l, int32_t t, int32_t r, int32_t b)
        : left(l), top(t), right(r), bottom(b) {}

    /**
     * @brief Calculate rectangle width.
     * @return Width in pixels (right - left)
     */
    int32_t width() const { return right - left; }

    /**
     * @brief Calculate rectangle height.
     * @return Height in pixels (bottom - top)
     */
    int32_t height() const { return bottom - top; }

    /**
     * @brief Check if this rectangle has a valid (non-zero) size.
     * @return true if width > 0 and height > 0
     */
    bool isValid() const { return width() > 0 && height() > 0; }

    /**
     * @brief Check if this rectangle is completely contained within another.
     * @param outer The outer rectangle to test against
     * @return true if this rectangle is fully within outer bounds
     */
    bool isContainedIn(const Rect& outer) const {
        return (outer.left <= left &&
                right <= outer.right &&
                outer.top <= top &&
                bottom <= outer.bottom);
    }
};

/**
 * @brief Platform-agnostic size.
 *
 * Represents width and height dimensions.
 */
struct Size {
    int32_t cx; ///< Width
    int32_t cy; ///< Height

    Size() : cx(0), cy(0) {}
    Size(int32_t cx, int32_t cy) : cx(cx), cy(cy) {}
};

/**
 * @brief Platform-agnostic key codes.
 *
 * Abstracted from platform-specific codes (e.g., Windows VK_*).
 * Extended with additional key codes as needed.
 */
enum class KeyCode : uint32_t {
    Unknown = 0,   ///< Unknown or unmapped key
    Escape = 0x1B, ///< Escape key
    Space = 0x20,  ///< Space bar
    // ... define key codes as needed
};

/**
 * @brief Mouse button identifiers.
 */
enum class MouseButton {
    Left,   ///< Left mouse button
    Right,  ///< Right mouse button
    Middle, ///< Middle mouse button (wheel click)
    X1,     ///< Extra button 1 (back)
    X2      ///< Extra button 2 (forward)
};

/**
 * @brief Platform-agnostic keyboard event.
 *
 * Represents a keyboard input event with key code, scan code, and metadata.
 */
struct KeyEvent {
    KeyCode key;        ///< Virtual key code
    bool isKeyDown;     ///< true for key press, false for key release
    bool isExtended;    ///< true if extended key (e.g., right Alt, right Ctrl)
    uint32_t scanCode;  ///< Hardware scan code
    uint32_t timestamp; ///< Event timestamp in milliseconds
    uint32_t flags;     ///< Platform-specific flags
    uintptr_t extraInfo;///< Extra information (for event identification)
};

/**
 * @brief Platform-agnostic mouse event.
 *
 * Represents a mouse input event with position, button, and metadata.
 */
struct MouseEvent {
    Point pt;            ///< Cursor position
    uint32_t mouseData;  ///< Wheel delta or button-specific data
    uint32_t flags;      ///< Event flags (button state, etc.)
    uint32_t time;       ///< Event timestamp in milliseconds
    uintptr_t extraInfo; ///< Extra information (for event identification)
    uint32_t message;    ///< Platform-specific message type
};

/**
 * @brief Window show commands.
 *
 * Specifies how a window should be displayed.
 */
enum class WindowShowCmd {
    Normal,    ///< Normal window state
    Maximized, ///< Maximized window
    Minimized, ///< Minimized window
    Unknown    ///< Unknown or default state
};

/**
 * @brief System metric identifiers.
 *
 * Used to query display/screen metrics.
 */
enum class SystemMetric {
    VirtualScreenWidth,  ///< Total width of all monitors
    VirtualScreenHeight, ///< Total height of all monitors
    ScreenWidth,         ///< Primary screen width
    ScreenHeight         ///< Primary screen height
};

/**
 * @brief Window Z-order positioning.
 *
 * Controls window stacking order.
 */
enum class ZOrder {
    Top,      ///< Move to top of Z-order
    Bottom,   ///< Move to bottom of Z-order
    TopMost,  ///< Make always-on-top
    NoTopMost ///< Remove always-on-top
};

/**
 * @brief Context for input injection operations.
 *
 * Tracks state during mouse dragging and other complex input sequences.
 */
struct InjectionContext {
    bool isDragging;      ///< true if currently performing a drag operation
    Point dragStartPos;   ///< Starting position of drag
};

/**
 * @brief Platform-agnostic message ID type.
 */
using MessageId = uint32_t;

/**
 * @brief Platform-agnostic message WPARAM type.
 */
using MessageWParam = uintptr_t;

/**
 * @brief Platform-agnostic message LPARAM type.
 */
using MessageLParam = intptr_t;

/**
 * @brief Platform-agnostic thread handle.
 */
using ThreadHandle = void*;

/**
 * @brief Platform-agnostic mutex handle.
 */
using MutexHandle = void*;

/**
 * @brief Platform-agnostic event handle (for synchronization).
 */
using EventHandle = void*;

/**
 * @brief Platform-agnostic module/library handle.
 */
using ModuleHandle = void*;

/**
 * @brief Platform-agnostic overlapped I/O handle.
 */
using OverlappedHandle = void*;

} // namespace yamy::platform
