#pragma once
#include "types.h"
#include <functional>

namespace yamy::platform {

// KeyEvent and MouseEvent are defined in types.h

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
