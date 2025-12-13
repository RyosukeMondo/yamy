#include "input_hook_win32.h"
#include "../../core/input/input_event.h" // Needed for constants if we used them, but we use raw values mostly
#include "hook.h"
#include "misc.h"
#include <process.h>

namespace yamy::platform {

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// InputHookWin32
//

InputHookWin32::InputHookWin32()
    : m_keyboardHandler(installKeyboardHook, InputHookWin32::keyboardDetour),
      m_mouseHandler(installMouseHook, InputHookWin32::mouseDetour),
      m_isEnabled(false),
      m_isInstalled(false),
      m_buttonPressed(false),
      m_dragging(false)
{
    m_msllHookCurrent.pt.x = 0;
    m_msllHookCurrent.pt.y = 0;
    m_msllHookCurrent.mouseData = 0;
    m_msllHookCurrent.flags = 0;
    m_msllHookCurrent.time = 0;
    m_msllHookCurrent.dwExtraInfo = 0;
}

InputHookWin32::~InputHookWin32()
{
    uninstall();
}

bool InputHookWin32::install(KeyCallback keyCallback, MouseCallback mouseCallback)
{
    if (m_isInstalled) return true;

    m_keyCallback = keyCallback;
    m_mouseCallback = mouseCallback;

    m_isEnabled = true;
    m_keyboardHandler.start(this);
    m_mouseHandler.start(this);

    m_isInstalled = true;
    return true;
}

void InputHookWin32::uninstall()
{
    if (!m_isInstalled) return;

    m_isEnabled = false;
    m_mouseHandler.stop();
    m_keyboardHandler.stop();

    m_keyCallback = nullptr;
    m_mouseCallback = nullptr;
    m_isInstalled = false;
}

bool InputHookWin32::isInstalled() const
{
    return m_isInstalled;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Detour Callbacks
//

unsigned int WINAPI InputHookWin32::keyboardDetour(void *i_context, WPARAM i_wParam, LPARAM i_lParam)
{
    InputHookWin32 *This = reinterpret_cast<InputHookWin32*>(i_context);
    if (!This || !This->m_keyCallback) return 0;

    KBDLLHOOKSTRUCT *kid = reinterpret_cast<KBDLLHOOKSTRUCT*>(i_lParam);
    
    if ((kid->flags & LLKHF_INJECTED) || !This->m_isEnabled) {
        return 0;
    } else {
        KeyEvent event;
        event.key = KeyCode::Unknown; // TODO: Map virtual key if needed, or rely on scanCode
        event.scanCode = kid->scanCode;
        event.flags = 0;
        if (kid->flags & LLKHF_UP) {
            event.flags |= 1; // Mark as break/up
            event.isKeyDown = false;
        } else {
            event.isKeyDown = true;
        }
        if (kid->flags & LLKHF_EXTENDED) {
            event.flags |= 2; // Mark as E0
        }
        event.timestamp = kid->time;
        event.extraInfo = kid->dwExtraInfo;

        bool handled = This->m_keyCallback(event);
        return handled ? 1 : 0;
    }
}

unsigned int WINAPI InputHookWin32::mouseDetour(void *i_context, WPARAM i_wParam, LPARAM i_lParam)
{
    InputHookWin32 *This = reinterpret_cast<InputHookWin32*>(i_context);
    if (!This || !This->m_mouseCallback) return 0;

    MSLLHOOKSTRUCT *mid = reinterpret_cast<MSLLHOOKSTRUCT*>(i_lParam);

    // Logic from original code to check for injection flag
    if ((mid->flags & LLMHF_INJECTED) || !This->m_isEnabled) {
        return 0;
    }

    MouseEvent event;
    event.pt.x = mid->pt.x;
    event.pt.y = mid->pt.y;
    event.mouseData = mid->mouseData;
    event.flags = mid->flags;
    event.time = mid->time;
    event.extraInfo = mid->dwExtraInfo;
    event.message = (uint32_t)i_wParam;

    bool handled = This->m_mouseCallback(event);
    return handled ? 1 : 0;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// InputHandler
//

unsigned int WINAPI InputHookWin32::InputHandler::run(void *i_this)
{
    reinterpret_cast<InputHandler*>(i_this)->run();
    _endthreadex(0);
    return 0;
}

InputHookWin32::InputHandler::InputHandler(INSTALL_HOOK i_installHook, INPUT_DETOUR i_inputDetour)
    : m_installHook(i_installHook), m_inputDetour(i_inputDetour), m_context(nullptr)
{
    // Simple check replacement
    m_hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_hThread = (HANDLE)_beginthreadex(nullptr, 0, run, this, CREATE_SUSPENDED, &m_threadId);
}

InputHookWin32::InputHandler::~InputHandler()
{
    CloseHandle(m_hEvent);
}

void InputHookWin32::InputHandler::run()
{
    MSG msg;

    m_installHook(m_inputDetour, m_context, true);
    PeekMessage(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    SetEvent(m_hEvent);

    while (GetMessage(&msg, nullptr, 0, 0)) {
        // nothing to do...
    }

    m_installHook(m_inputDetour, m_context, false);

    return;
}

int InputHookWin32::InputHandler::start(void *i_context)
{
    m_context = i_context;
    ResumeThread(m_hThread);
    WaitForSingleObject(m_hEvent, INFINITE);
    return 0;
}

int InputHookWin32::InputHandler::stop()
{
    PostThreadMessage(m_threadId, WM_QUIT, 0, 0);
    WaitForSingleObject(m_hThread, INFINITE);
    return 0;
}

IInputHook* createInputHook() {
    return new InputHookWin32();
}

} // namespace yamy::platform
