//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_input.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>
#include <process.h>


unsigned int Engine::injectInput(const KEYBOARD_INPUT_DATA *i_kid, const KBDLLHOOKSTRUCT *i_kidRaw)
{
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
				return 1;
			} else {
				kid[0].mi.mouseData = WHEEL_DELTA;
				kid[0].mi.dwFlags = MOUSEEVENTF_WHEEL;
			}
			break;
		case 5:
			if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
				return 1;
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
				return 1;
			} else {
				kid[0].mi.mouseData = WHEEL_DELTA;
				kid[0].mi.dwFlags = MOUSEEVENTF_HWHEEL;
			}
			break;
		case 9:
			if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
				return 1;
			} else {
				kid[0].mi.mouseData = -WHEEL_DELTA;
				kid[0].mi.dwFlags = MOUSEEVENTF_HWHEEL;
			}
			break;
		default:
			return 1;
			break;
		}
		if (!(i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) &&
			i_kid->MakeCode != 4 && i_kid->MakeCode != 5 &&
			i_kid->MakeCode != 8 && i_kid->MakeCode != 9) {
			HWND hwnd;
			POINT pt;

			if (GetCursorPos(&pt) && (hwnd = WindowFromPoint(pt))) {
				_TCHAR className[GANA_MAX_ATOM_LENGTH];
				if (GetClassName(hwnd, className, NUMBER_OF(className))) {
					if (_tcsicmp(className, _T("ConsoleWindowClass")) == 0) {
						SetForegroundWindow(hwnd);
					}
				}
			}
			if (m_dragging) {
				kid[0].mi.dx = 65535 * m_msllHookCurrent.pt.x / GetSystemMetrics(SM_CXVIRTUALSCREEN);
				kid[0].mi.dy = 65535 * m_msllHookCurrent.pt.y / GetSystemMetrics(SM_CYVIRTUALSCREEN);
				kid[0].mi.dwFlags |= MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;

				kid[1].type = INPUT_MOUSE;
				kid[1].mi.dx = 65535 * pt.x / GetSystemMetrics(SM_CXVIRTUALSCREEN);
				kid[1].mi.dy = 65535 * pt.y / GetSystemMetrics(SM_CYVIRTUALSCREEN);
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
	return 1;
}


// pop all pressed key on win32
void Engine::keyboardResetOnWin32()
{
	for (Keyboard::KeyIterator
			i = m_setting->m_keyboard.getKeyIterator();  *i; ++ i) {
		if ((*i)->m_isPressedOnWin32)
			generateKeyEvent((*i), false, true);
	}
}
