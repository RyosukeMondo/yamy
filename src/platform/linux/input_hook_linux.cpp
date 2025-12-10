#include "core/platform/input_hook_interface.h"
#include <iostream>

namespace yamy::platform {

class InputHookLinux : public IInputHook {
public:
    bool install(KeyCallback keyCallback, MouseCallback mouseCallback) override {
        std::cerr << "[STUB] InputHook::install()" << std::endl;
        return true;
    }

    void uninstall() override {
        std::cerr << "[STUB] InputHook::uninstall()" << std::endl;
    }

    bool isInstalled() const override {
        return false;
    }
};

IInputHook* createInputHook() {
    std::cerr << "[Linux] Creating stub InputHook" << std::endl;
    return new InputHookLinux();
}

} // namespace yamy::platform
