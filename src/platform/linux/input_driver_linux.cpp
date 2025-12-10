#include "core/platform/input_driver_interface.h"
#include <iostream>

namespace yamy::platform {

class InputDriverLinux : public IInputDriver {
public:
    bool open(void *readEvent) override {
        std::cerr << "[STUB] InputDriver::open()" << std::endl;
        return true;
    }

    void close() override {
        std::cerr << "[STUB] InputDriver::close()" << std::endl;
    }

    void manageExtension(const std::string& dllName, const std::string& dependDllName, bool load, void **moduleHandle) override {
        std::cerr << "[STUB] InputDriver::manageExtension()" << std::endl;
        if (moduleHandle) *moduleHandle = nullptr;
    }
};

IInputDriver* createInputDriver() {
    std::cerr << "[Linux] Creating stub InputDriver" << std::endl;
    return new InputDriverLinux();
}

} // namespace yamy::platform
