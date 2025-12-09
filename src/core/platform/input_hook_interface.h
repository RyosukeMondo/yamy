#pragma once
#include "types.h"
#include <functional>

namespace yamy::platform {

struct KeyEvent {
    KeyCode key;
    bool isKeyDown;
    uint32_t scanCode;
    uint32_t timestamp;
};

using KeyCallback = std::function<bool(const KeyEvent&)>;

class IInputHook {
public:
    virtual ~IInputHook() = default;

    virtual bool install(KeyCallback callback) = 0;
    virtual void uninstall() = 0;
    virtual bool isInstalled() const = 0;
};

IInputHook* createInputHook();

} // namespace yamy::platform
