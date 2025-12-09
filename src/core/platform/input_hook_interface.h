#pragma once
#include "types.h"
#include <functional>

namespace yamy::platform {

struct KeyEvent {
    KeyCode key;
    bool isKeyDown;
    uint32_t scanCode;
    uint32_t flags;
    uintptr_t extraInfo;
    uint32_t timestamp;
};

struct MouseEvent {
    Point pt;
    uint32_t mouseData;
    uint32_t flags;
    uint32_t time;
    uintptr_t extraInfo;
    uint32_t message;
};

using KeyCallback = std::function<bool(const KeyEvent&)>;
using MouseCallback = std::function<bool(const MouseEvent&)>;

class IInputHook {
public:
    virtual ~IInputHook() = default;

    virtual bool install(KeyCallback keyCallback, MouseCallback mouseCallback) = 0;
    virtual void uninstall() = 0;
    virtual bool isInstalled() const = 0;
};

IInputHook* createInputHook();

} // namespace yamy::platform
