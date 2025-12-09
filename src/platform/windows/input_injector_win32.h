#pragma once
#ifndef _INPUT_INJECTOR_WIN32_H
#define _INPUT_INJECTOR_WIN32_H

#include "../../core/input/input_injector.h"
#include "../../core/window/window_system.h"
#include "../../core/platform/input_injector_interface.h"
#include <windows.h>

class InputInjectorWin32 : public InputInjector, public yamy::platform::IInputInjector {
    WindowSystem *m_windowSystem;
public:
    InputInjectorWin32(WindowSystem *ws) : m_windowSystem(ws) {}
    void inject(const KEYBOARD_INPUT_DATA *data, const InjectionContext &ctx, const void *rawData = 0) override;

    // IInputInjector implementation
    void keyDown(yamy::platform::KeyCode key) override;
    void keyUp(yamy::platform::KeyCode key) override;

    void injectKey(const yamy::platform::KeyEvent& event) override;

    void mouseMove(int32_t dx, int32_t dy) override;
    void mouseButton(yamy::platform::MouseButton button, bool down) override;
    void mouseWheel(int32_t delta) override;
};

#endif // !_INPUT_INJECTOR_WIN32_H
