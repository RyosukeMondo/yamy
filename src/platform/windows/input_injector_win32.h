#pragma once
#ifndef _INPUT_INJECTOR_WIN32_H
#define _INPUT_INJECTOR_WIN32_H

#include "../../core/platform/input_injector_interface.h"
#include "../../core/platform/window_system_interface.h"
#include "../../core/input/input_event.h" // For KEYBOARD_INPUT_DATA
#include <windows.h>

namespace yamy::platform {

class InputInjectorWin32 : public IInputInjector {
    IWindowSystem *m_windowSystem;
public:
    InputInjectorWin32(IWindowSystem *ws) : m_windowSystem(ws) {}

    void inject(const KEYBOARD_INPUT_DATA *data, const InjectionContext &ctx, const void *rawData = 0) override;

    // New methods
    void keyDown(KeyCode key) override;
    void keyUp(KeyCode key) override;
    void mouseMove(int32_t dx, int32_t dy) override;
    void mouseButton(MouseButton button, bool down) override;
    void mouseWheel(int32_t delta) override;
};

} // namespace yamy::platform

#endif // !_INPUT_INJECTOR_WIN32_H
