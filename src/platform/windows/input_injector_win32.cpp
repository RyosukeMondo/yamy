#include "input_injector_win32.h"
#include <tchar.h>

void InputInjectorWin32::inject(const KEYBOARD_INPUT_DATA *i_kid, const InjectionContext &ctx, const void *rawData)
{
    const KBDLLHOOKSTRUCT *i_kidRaw = static_cast<const KBDLLHOOKSTRUCT*>(rawData);

    if (i_kid->Flags & KEYBOARD_INPUT_DATA::E1) {
        INPUT kid[2];
        int count = 1;

        kid[0].type = INPUT_MOUSE;
        kid[0].mi.dx = 0;
        kid[0].mi.dy = 0;
        kid[0].mi.time = 0;
        kid[0].mi.mouseData = 0;
        kid[0].mi.dwExtraInfo = 0;
        switch (i_kid->MakeCode) {
        case 1:
            if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
                kid[0].mi.dwFlags = MOUSEEVENTF_LEFTUP;
            } else {
                kid[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            }
            break;
        case 2:
            if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
                kid[0].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            } else {
                kid[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            }
            break;
        case 3:
            if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
                kid[0].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
            } else {
                kid[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
            }
            break;
        case 4:
            if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
                return;
            } else {
                kid[0].mi.mouseData = WHEEL_DELTA;
                kid[0].mi.dwFlags = MOUSEEVENTF_WHEEL;
            }
            break;
        case 5:
            if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
                return;
            } else {
                kid[0].mi.mouseData = -WHEEL_DELTA;
                kid[0].mi.dwFlags = MOUSEEVENTF_WHEEL;
            }
            break;
        case 6:
            kid[0].mi.mouseData = XBUTTON1;
            if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
                kid[0].mi.dwFlags = MOUSEEVENTF_XUP;
            } else {
                kid[0].mi.dwFlags = MOUSEEVENTF_XDOWN;
            }
            break;
        case 7:
            kid[0].mi.mouseData = XBUTTON2;
            if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
                kid[0].mi.dwFlags = MOUSEEVENTF_XUP;
            } else {
                kid[0].mi.dwFlags = MOUSEEVENTF_XDOWN;
            }
            break;
        case 8:
            if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
                return;
            } else {
                kid[0].mi.mouseData = WHEEL_DELTA;
                kid[0].mi.dwFlags = MOUSEEVENTF_HWHEEL;
            }
            break;
        case 9:
            if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
                return;
            } else {
                kid[0].mi.mouseData = -WHEEL_DELTA;
                kid[0].mi.dwFlags = MOUSEEVENTF_HWHEEL;
            }
            break;
        default:
            return;
            break;
        }
        if (!(i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) &&
            i_kid->MakeCode != 4 && i_kid->MakeCode != 5 &&
            i_kid->MakeCode != 8 && i_kid->MakeCode != 9) {
            WindowSystem::WindowHandle hwnd;
            WindowPoint pt;

            if (m_windowSystem->getCursorPos(&pt) && (hwnd = m_windowSystem->windowFromPoint(pt))) {
                if (m_windowSystem->isConsoleWindow(hwnd)) {
                    m_windowSystem->setForegroundWindow(hwnd);
                }
            }
            if (ctx.isDragging) {
                int cx = m_windowSystem->getSystemMetrics(SystemMetric::VirtualScreenWidth);
                int cy = m_windowSystem->getSystemMetrics(SystemMetric::VirtualScreenHeight);
                // Avoid division by zero if metrics fail
                if (cx == 0) cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                if (cy == 0) cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);

                kid[0].mi.dx = 65535 * ctx.dragStartPos.x / cx;
                kid[0].mi.dy = 65535 * ctx.dragStartPos.y / cy;
                kid[0].mi.dwFlags |= MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;

                kid[1].type = INPUT_MOUSE;
                kid[1].mi.dx = 65535 * pt.x / cx;
                kid[1].mi.dy = 65535 * pt.y / cy;
                kid[1].mi.time = 0;
                kid[1].mi.mouseData = 0;
                kid[1].mi.dwExtraInfo = 0;
                kid[1].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;

                count = 2;
            }
        }
        SendInput(count, &kid[0], sizeof(kid[0]));
    } else {
        INPUT kid;

        kid.type = INPUT_KEYBOARD;
        kid.ki.wVk = 0;
        kid.ki.wScan = i_kid->MakeCode;
        kid.ki.dwFlags = KEYEVENTF_SCANCODE;
        kid.ki.time = i_kidRaw ? i_kidRaw->time : 0;
        kid.ki.dwExtraInfo = i_kidRaw ? i_kidRaw->dwExtraInfo : 0;
        if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
            kid.ki.dwFlags |= KEYEVENTF_KEYUP;
        }
        if (i_kid->Flags & KEYBOARD_INPUT_DATA::E0) {
            kid.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        }
        SendInput(1, &kid, sizeof(kid));
    }
}
