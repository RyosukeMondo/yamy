#pragma once
#include "types.h"
#include <cstdint>

class KEYBOARD_INPUT_DATA;

namespace yamy::platform {

class IInputInjector {
public:
    virtual ~IInputInjector() = default;

    virtual void inject(const KEYBOARD_INPUT_DATA *data, const InjectionContext &ctx, const void *rawData = 0) = 0;

    virtual void keyDown(KeyCode key) = 0;
    virtual void keyUp(KeyCode key) = 0;
    virtual void mouseMove(int32_t dx, int32_t dy) = 0;
    virtual void mouseButton(MouseButton button, bool down) = 0;
    virtual void mouseWheel(int32_t delta) = 0;
};

class IWindowSystem;
IInputInjector* createInputInjector(IWindowSystem* windowSystem);

} // namespace yamy::platform
