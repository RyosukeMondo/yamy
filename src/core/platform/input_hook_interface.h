#pragma once
#include "types.h"
#include <functional>

namespace yamy::platform {

// KeyEvent is defined in types.h

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
