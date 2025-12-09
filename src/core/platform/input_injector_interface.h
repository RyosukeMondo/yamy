#pragma once
#include "types.h"
#include <cstdint>

namespace yamy::platform {

class IInputInjector {
public:
    virtual ~IInputInjector() = default;

    // Keyboard
    virtual void keyDown(KeyCode key) = 0;
    virtual void keyUp(KeyCode key) = 0;

    // Advanced Injection
    virtual void injectKey(const KeyEvent& event) = 0;

    // Mouse
    virtual void mouseMove(int32_t dx, int32_t dy) = 0;
    virtual void mouseButton(MouseButton button, bool down) = 0;
    virtual void mouseWheel(int32_t delta) = 0;
};

IInputInjector* createInputInjector();

} // namespace yamy::platform
