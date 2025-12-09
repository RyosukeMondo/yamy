#pragma once
#include <cstdint>
#include <string>

namespace yamy::platform {

class IInputDriver {
public:
    virtual ~IInputDriver() = default;

    virtual bool open(void *readEvent) = 0;
    virtual void close() = 0;

    virtual void manageExtension(const std::string& dllName, const std::string& dependDllName, bool load, void **moduleHandle) = 0;
};

IInputDriver* createInputDriver();

} // namespace yamy::platform
