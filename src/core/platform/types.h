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
};

// Key codes (abstracted from Windows VK_*)
enum class KeyCode : uint32_t {
    Unknown = 0,
    Escape = 0x1B,
    Space = 0x20,
    // Add more as needed
};

// Mouse buttons
enum class MouseButton {
    Left,
    Right,
    Middle,
    X1,
    X2
};

// Input Event
struct KeyEvent {
    KeyCode key;
    bool isKeyDown;
    bool isExtended;
    uint32_t scanCode;
    uint32_t timestamp;
};

} // namespace yamy::platform
