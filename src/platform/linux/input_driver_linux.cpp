#include "core/platform/input_driver_interface.h"
#include <iostream>

namespace yamy::platform {

class InputDriverLinux : public IInputDriver {
public:
    bool initialize() override {
        std::cerr << "[STUB] InputDriver::initialize() - not supported on Linux" << std::endl;
        return true;
    }

    void shutdown() override {
        std::cerr << "[STUB] InputDriver::shutdown()" << std::endl;
    }

    void processEvents() override {
        // No-op for stub
    }

    bool isKeyPressed(uint32_t key) const override {
        return false;
    }
};

IInputDriver* createInputDriver() {
    std::cerr << "[Linux] Creating stub InputDriver" << std::endl;
    return new InputDriverLinux();
}

} // namespace yamy::platform
