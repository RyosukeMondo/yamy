#pragma once
#include <cstdint>

namespace yamy::platform {

// Platform-agnostic window handle
using WindowHandle = void*;

// Platform-agnostic point
struct Point {
    int32_t x;
    int32_t y;

    Point() : x(0), y(0) {}
    Point(int32_t x, int32_t y) : x(x), y(y) {}
};

// Platform-agnostic rectangle
struct Rect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;

    Rect() : left(0), top(0), right(0), bottom(0) {}
    Rect(int32_t l, int32_t t, int32_t r, int32_t b)
        : left(l), top(t), right(r), bottom(b) {}

    int32_t width() const { return right - left; }
    int32_t height() const { return bottom - top; }

    /// Check if this rectangle is completely contained within another rectangle
    bool isContainedIn(const Rect& outer) const {
        return (outer.left <= left &&
                right <= outer.right &&
                outer.top <= top &&
                bottom <= outer.bottom);
    }
};

// Platform-agnostic size
struct Size {
    int32_t cx;
    int32_t cy;

    Size() : cx(0), cy(0) {}
    Size(int32_t cx, int32_t cy) : cx(cx), cy(cy) {}
};

// Key codes (abstracted from Windows VK_*)
enum class KeyCode : uint32_t {
    Unknown = 0,
    Escape = 0x1B,
    Space = 0x20,
    // ... define key codes as needed
};

// Mouse buttons
enum class MouseButton {
    Left,
    Right,
    Middle,
    X1,
    X2
};

// Input Events
struct KeyEvent {
    KeyCode key;
    bool isKeyDown;
    bool isExtended;
    uint32_t scanCode;
    uint32_t timestamp;
    uint32_t flags;
    uintptr_t extraInfo;
};

struct MouseEvent {
    Point pt;
    uint32_t mouseData;
    uint32_t flags;
    uint32_t time;
    uintptr_t extraInfo;
    uint32_t message;
};

// Additional enums from incoming branch
enum class WindowShowCmd {
    Normal,
    Maximized,
    Minimized,
    Unknown
};

enum class SystemMetric {
    VirtualScreenWidth,
    VirtualScreenHeight,
    ScreenWidth,
    ScreenHeight
};

enum class ZOrder {
    Top,
    Bottom,
    TopMost,
    NoTopMost
};

struct InjectionContext {
    bool isDragging;
    Point dragStartPos;
};

// Platform-agnostic message types
using MessageId = uint32_t;
using MessageWParam = uintptr_t;
using MessageLParam = intptr_t;

// Platform-agnostic threading types
using ThreadHandle = void*;
using MutexHandle = void*;
using EventHandle = void*;
using ModuleHandle = void*;
using OverlappedHandle = void*;

} // namespace yamy::platform
