#pragma once
#ifndef _INPUT_HOOK_WIN32_H
#define _INPUT_HOOK_WIN32_H

#include "../../core/platform/input_hook_interface.h"
#include "hook.h"
#include <windows.h>

namespace yamy::platform {

class InputHookWin32 : public IInputHook
{
public:
    InputHookWin32();
    virtual ~InputHookWin32();

    // IInputHook implementation
    bool install(KeyCallback keyCallback, MouseCallback mouseCallback) override;
    void uninstall() override;
    bool isInstalled() const override;

    // Callback for Windows hook
    static unsigned int WINAPI keyboardDetour(void *i_context, WPARAM i_wParam, LPARAM i_lParam);
    static unsigned int WINAPI mouseDetour(void *i_context, WPARAM i_wParam, LPARAM i_lParam);

private:
    // Internal Thread Handler to manage hook lifetime
    class InputHandler {
    public:
        typedef int (*INSTALL_HOOK)(INPUT_DETOUR i_keyboardDetour, void *i_context, bool i_install);

        static unsigned int WINAPI run(void *i_this);

        InputHandler(INSTALL_HOOK i_installHook, INPUT_DETOUR i_inputDetour);
        ~InputHandler();

        int start(void *i_context);
        int stop();

    private:
        void run();

        unsigned m_threadId;
        HANDLE m_hThread;
        HANDLE m_hEvent;
        INSTALL_HOOK m_installHook;
        INPUT_DETOUR m_inputDetour;
        void *m_context;
    };

    InputHandler m_keyboardHandler;
    InputHandler m_mouseHandler;

    KeyCallback m_keyCallback;
    MouseCallback m_mouseCallback;

    bool m_isEnabled;
    bool m_isInstalled;

    // Mouse state
    MSLLHOOKSTRUCT m_msllHookCurrent;
    bool m_buttonPressed;
    bool m_dragging;
};

} // namespace yamy::platform

#endif // !_INPUT_HOOK_WIN32_H
