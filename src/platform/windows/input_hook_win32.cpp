#include "input_hook_win32.h"
#include "../../core/engine/engine.h"
#include "../../core/input/input_event.h"
#include "hook.h"
#include "misc.h"
#include <process.h>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// InputHookWin32
//

InputHookWin32::InputHookWin32()
    : m_keyboardHandler(installKeyboardHook, InputHookWin32::keyboardDetour),
      m_mouseHandler(installMouseHook, InputHookWin32::mouseDetour),
      m_engine(NULL),
      m_isEnabled(true),
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
    stop();
}

bool InputHookWin32::start(Engine *i_engine)
{
    m_engine = i_engine;
    m_keyboardHandler.start(this);
    m_mouseHandler.start(this);
    return true;
}

bool InputHookWin32::stop()
{
    m_mouseHandler.stop();
    m_keyboardHandler.stop();
    m_engine = NULL;
    return true;
}

bool InputHookWin32::pause()
{
    // Implementation dependent on if we want to remove hooks or just ignore
    // Currently Engine uses stop/start to pause/resume
    return stop();
}

bool InputHookWin32::resume()
{
    if (m_engine)
        return start(m_engine);
    return false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Detour Callbacks
//

unsigned int WINAPI InputHookWin32::keyboardDetour(void *i_context, WPARAM i_wParam, LPARAM i_lParam)
{
    InputHookWin32 *This = reinterpret_cast<InputHookWin32*>(i_context);
    if (!This || !This->m_engine) return 0;

    KBDLLHOOKSTRUCT *kid = reinterpret_cast<KBDLLHOOKSTRUCT*>(i_lParam);
    
    if ((kid->flags & LLKHF_INJECTED) || !This->m_isEnabled) {
        return 0;
    } else {
        KEYBOARD_INPUT_DATA data;
        data.UnitId = 0;
        data.MakeCode = (USHORT)kid->scanCode;
        data.Flags = 0;
        if (kid->flags & LLKHF_UP) {
            data.Flags |= KEYBOARD_INPUT_DATA::BREAK;
        }
        if (kid->flags & LLKHF_EXTENDED) {
            data.Flags |= KEYBOARD_INPUT_DATA::E0;
        }
        data.Reserved = 0;
        data.ExtraInformation = 0;

        This->m_engine->pushInputEvent(data);
        return 1;
    }
}

unsigned int WINAPI InputHookWin32::mouseDetour(void *i_context, WPARAM i_wParam, LPARAM i_lParam)
{
    InputHookWin32 *This = reinterpret_cast<InputHookWin32*>(i_context);
    if (!This || !This->m_engine) return 0;

    MSLLHOOKSTRUCT *mid = reinterpret_cast<MSLLHOOKSTRUCT*>(i_lParam);
    const Setting *setting = This->m_engine->getSetting();

    if ((mid->flags & LLMHF_INJECTED) || !This->m_isEnabled || !setting || !setting->m_mouseEvent) {
        return 0;
    } else {
        KEYBOARD_INPUT_DATA data;

        data.UnitId = 0;
        data.Flags = KEYBOARD_INPUT_DATA::E1;
        data.Reserved = 0;
        data.ExtraInformation = 0x59414D59; // "YAMY" magic for mouse events
        
        switch (i_wParam) {
        case WM_LBUTTONUP:
            data.Flags |= KEYBOARD_INPUT_DATA::BREAK;
        case WM_LBUTTONDOWN:
            data.MakeCode = 1;
            break;
        case WM_RBUTTONUP:
            data.Flags |= KEYBOARD_INPUT_DATA::BREAK;
        case WM_RBUTTONDOWN:
            data.MakeCode = 2;
            break;
        case WM_MBUTTONUP:
            data.Flags |= KEYBOARD_INPUT_DATA::BREAK;
        case WM_MBUTTONDOWN:
            data.MakeCode = 3;
            break;
        case WM_MOUSEWHEEL:
            if (mid->mouseData & (1<<31)) {
                data.MakeCode = 5;
            } else {
                data.MakeCode = 4;
            }
            break;
        case WM_XBUTTONUP:
            data.Flags |= KEYBOARD_INPUT_DATA::BREAK;
        case WM_XBUTTONDOWN:
            switch ((mid->mouseData >> 16) & 0xFFFFU) {
            case XBUTTON1:
                data.MakeCode = 6;
                break;
            case XBUTTON2:
                data.MakeCode = 7;
                break;
            default:
                return 0;
                break;
            }
            break;
        case WM_MOUSEHWHEEL:
            if (mid->mouseData & (1<<31)) {
                data.MakeCode = 9;
            } else {
                data.MakeCode = 8;
            }
            break;
        case WM_MOUSEMOVE: {
            if (!g_hookData) return 0;
            
            LONG dx = mid->pt.x - g_hookData->m_mousePos.x;
            LONG dy = mid->pt.y - g_hookData->m_mousePos.y;
            HWND target = (HWND)(ULONG_PTR)(g_hookData->m_hwndMouseHookTarget);

            LONG dr = 0;
            dr += (mid->pt.x - This->m_msllHookCurrent.pt.x) * (mid->pt.x - This->m_msllHookCurrent.pt.x);
            dr += (mid->pt.y - This->m_msllHookCurrent.pt.y) * (mid->pt.y - This->m_msllHookCurrent.pt.y);
            
            if (This->m_buttonPressed && !This->m_dragging && setting->m_dragThreshold &&
                (setting->m_dragThreshold * setting->m_dragThreshold < dr)) {
                data.MakeCode = 0;
                This->m_dragging = true;
                This->m_engine->pushInputEvent(data);
            }

            switch (g_hookData->m_mouseHookType) {
            case MouseHookType_Wheel:
                mouse_event(MOUSEEVENTF_WHEEL, 0, 0,
                            g_hookData->m_mouseHookParam * dy, 0);
                return 1;
                break;
            case MouseHookType_WindowMove: {
                RECT curRect;

                if (!GetWindowRect(target, &curRect))
                    return 0;

                if (g_hookData->m_mouseHookParam < 0) {
                    HWND parent = GetParent(target);
                    POINT p = {curRect.left, curRect.top};

                    if (parent == NULL || !ScreenToClient(parent, &p))
                        return 0;

                    curRect.left = p.x;
                    curRect.top = p.y;
                }

                SetWindowPos(target, NULL,
                             curRect.left + dx,
                             curRect.top + dy,
                             0, 0,
                             SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE |
                             SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
                g_hookData->m_mousePos = mid->pt;
                return 0;
                break;
            }
            case MouseHookType_None:
            default:
                return 0;
                break;
            }
        }
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
        default:
            return 0;
            break;
        }

        if (data.Flags & KEYBOARD_INPUT_DATA::BREAK) {
            This->m_buttonPressed = false;
            if (This->m_dragging) {
                KEYBOARD_INPUT_DATA kid2;

                This->m_dragging = false;
                kid2.UnitId = 0;
                kid2.Flags = KEYBOARD_INPUT_DATA::E1 | KEYBOARD_INPUT_DATA::BREAK;
                kid2.Reserved = 0;
                kid2.ExtraInformation = 0;
                kid2.MakeCode = 0;
                This->m_engine->pushInputEvent(kid2);
            }
        } else if (i_wParam != WM_MOUSEWHEEL && i_wParam != WM_MOUSEHWHEEL) {
            This->m_buttonPressed = true;
            This->m_msllHookCurrent = *mid;
        }

        This->m_engine->pushInputEvent(data);

        if (i_wParam == WM_MOUSEWHEEL || i_wParam == WM_MOUSEHWHEEL) {
            data.UnitId = 0;
            data.Flags |= KEYBOARD_INPUT_DATA::BREAK;
            data.Reserved = 0;
            data.ExtraInformation = 0;
            This->m_engine->pushInputEvent(data);
        }

        return 1;
    }
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
    : m_installHook(i_installHook), m_inputDetour(i_inputDetour), m_context(NULL)
{
    CHECK_TRUE(m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL));
    CHECK_TRUE(m_hThread = (HANDLE)_beginthreadex(NULL, 0, run, this, CREATE_SUSPENDED, &m_threadId));
}

InputHookWin32::InputHandler::~InputHandler()
{
    CloseHandle(m_hEvent);
}

void InputHookWin32::InputHandler::run()
{
    MSG msg;

    CHECK_FALSE(m_installHook(m_inputDetour, m_context, true));
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
    SetEvent(m_hEvent);

    while (GetMessage(&msg, NULL, 0, 0)) {
        // nothing to do...
    }

    CHECK_FALSE(m_installHook(m_inputDetour, m_context, false));

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
