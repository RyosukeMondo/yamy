#include "core/platform/input_hook_interface.h"
#include <iostream>

namespace yamy::platform {

class InputHookLinux : public IInputHook {
    bool install(KeyCallback callback) override {
        std::cerr << "[STUB] InputHook::install() - not supported on Linux" << std::endl;
        return false;
    }

    void uninstall() override {
        std::cerr << "[STUB] InputHook::uninstall() - not supported on Linux" << std::endl;
    }

    bool isInstalled() const override { return false; }
};

IInputHook* createInputHook() {
    std::cerr << "[Linux] Creating stub InputHook" << std::endl;
    return new InputHookLinux();
}

} // namespace yamy::platform
