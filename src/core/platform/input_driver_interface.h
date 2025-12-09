#pragma once
#include <cstdint>

namespace yamy::platform {

class IInputDriver {
public:
    virtual ~IInputDriver() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual void processEvents() = 0;
    virtual bool isKeyPressed(uint32_t key) const = 0;
};

IInputDriver* createInputDriver();

} // namespace yamy::platform
