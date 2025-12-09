#pragma once
#include <cstdint>
#include <string>

namespace yamy::platform {

class IInputDriver {
public:
    virtual ~IInputDriver() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual void processEvents() = 0;
    virtual bool isKeyPressed(uint32_t key) const = 0;

    // Extensions
    virtual void manageExtension(const std::string& dllName, const std::string& dependDllName, bool load, void** moduleHandle) = 0;
};

IInputDriver* createInputDriver();

} // namespace yamy::platform
