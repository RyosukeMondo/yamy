#include "core/platform/input_injector_interface.h"
#include <iostream>

namespace yamy::platform {

class InputInjectorLinux : public IInputInjector {
public:
    // Keyboard
    void keyDown(KeyCode key) override {
        std::cerr << "[STUB] keyDown(" << static_cast<uint32_t>(key) << ")" << std::endl;
    }

    void keyUp(KeyCode key) override {
        std::cerr << "[STUB] keyUp(" << static_cast<uint32_t>(key) << ")" << std::endl;
    }

    // Mouse
    void mouseMove(int32_t dx, int32_t dy) override {
        std::cerr << "[STUB] mouseMove(" << dx << ", " << dy << ")" << std::endl;
    }

    void mouseButton(MouseButton button, bool down) override {
        std::cerr << "[STUB] mouseButton(" << static_cast<int>(button) << ", " << (down ? "down" : "up") << ")" << std::endl;
    }

    void mouseWheel(int32_t delta) override {
        std::cerr << "[STUB] mouseWheel(" << delta << ")" << std::endl;
    }
};

IInputInjector* createInputInjector() {
    std::cerr << "[Linux] Creating stub InputInjector" << std::endl;
    return new InputInjectorLinux();
}

} // namespace yamy::platform
