#ifndef _INPUT_INJECTOR_WIN32_H
#define _INPUT_INJECTOR_WIN32_H

#include "../../core/input/input_injector.h"
#include "../../core/window/window_system.h"
#include <windows.h>

class InputInjectorWin32 : public InputInjector {
    WindowSystem *m_windowSystem;
public:
    InputInjectorWin32(WindowSystem *ws) : m_windowSystem(ws) {}
    void inject(const KEYBOARD_INPUT_DATA *data, const InjectionContext &ctx, const void *rawData = 0) override;
};

#endif // !_INPUT_INJECTOR_WIN32_H
