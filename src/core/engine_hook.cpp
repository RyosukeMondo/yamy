//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_hook.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>
#include <process.h>


unsigned int WINAPI Engine::keyboardDetour(Engine *i_this, WPARAM i_wParam, LPARAM i_lParam)
{
	return i_this->keyboardDetour(reinterpret_cast<KBDLLHOOKSTRUCT*>(i_lParam));
}

unsigned int Engine::keyboardDetour(KBDLLHOOKSTRUCT *i_kid)
{
#if 0
	Acquire a(&m_log, 1);
	m_log << std::hex
	<< _T("keyboardDetour: vkCode=") << i_kid->vkCode
	<< _T(" scanCode=") << i_kid->scanCode
	<< _T(" flags=") << i_kid->flags << std::endl;
#endif
	if ((i_kid->flags & LLKHF_INJECTED) || !m_isEnabled) {
		return 0;
	} else {
		Key key;
		KEYBOARD_INPUT_DATA kid;

		kid.UnitId = 0;
		kid.MakeCode = (USHORT)i_kid->scanCode;
		kid.Flags = 0;
		if (i_kid->flags & LLKHF_UP) {
			kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
		}
		if (i_kid->flags & LLKHF_EXTENDED) {
			kid.Flags |= KEYBOARD_INPUT_DATA::E0;
		}
		kid.Reserved = 0;
		kid.ExtraInformation = 0;

		WaitForSingleObject(m_queueMutex, INFINITE);
		m_inputQueue->push_back(kid);
		SetEvent(m_readEvent);
		ReleaseMutex(m_queueMutex);
		return 1;
	}
}

unsigned int WINAPI Engine::mouseDetour(Engine *i_this, WPARAM i_wParam, LPARAM i_lParam)
{
	return i_this->mouseDetour(i_wParam, reinterpret_cast<MSLLHOOKSTRUCT*>(i_lParam));
}

unsigned int Engine::mouseDetour(WPARAM i_message, MSLLHOOKSTRUCT *i_mid)
{
	if (i_mid->flags & LLMHF_INJECTED || !m_isEnabled || !m_setting || !m_setting->m_mouseEvent) {
		return 0;
	} else {
		KEYBOARD_INPUT_DATA kid;

		kid.UnitId = 0;
		kid.Flags = KEYBOARD_INPUT_DATA::E1;
		kid.Reserved = 0;
		kid.ExtraInformation = 0x59414D59; // "YAMY" magic for mouse events
		switch (i_message) {
		case WM_LBUTTONUP:
			kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
		case WM_LBUTTONDOWN:
			kid.MakeCode = 1;
			break;
		case WM_RBUTTONUP:
			kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
		case WM_RBUTTONDOWN:
			kid.MakeCode = 2;
			break;
		case WM_MBUTTONUP:
			kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
		case WM_MBUTTONDOWN:
			kid.MakeCode = 3;
			break;
		case WM_MOUSEWHEEL:
			if (i_mid->mouseData & (1<<31)) {
				kid.MakeCode = 5;
			} else {
				kid.MakeCode = 4;
			}
			break;
		case WM_XBUTTONUP:
			kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
		case WM_XBUTTONDOWN:
			switch ((i_mid->mouseData >> 16) & 0xFFFFU) {
			case XBUTTON1:
				kid.MakeCode = 6;
				break;
			case XBUTTON2:
				kid.MakeCode = 7;
				break;
			default:
				return 0;
				break;
			}
			break;
		case WM_MOUSEHWHEEL:
			if (i_mid->mouseData & (1<<31)) {
				kid.MakeCode = 9;
			} else {
				kid.MakeCode = 8;
			}
			break;
		case WM_MOUSEMOVE: {
			LONG dx = i_mid->pt.x - g_hookData->m_mousePos.x;
			LONG dy = i_mid->pt.y - g_hookData->m_mousePos.y;
			HWND target = (HWND)(ULONG_PTR)(g_hookData->m_hwndMouseHookTarget);

			LONG dr = 0;
			dr += (i_mid->pt.x - m_msllHookCurrent.pt.x) * (i_mid->pt.x - m_msllHookCurrent.pt.x);
			dr += (i_mid->pt.y - m_msllHookCurrent.pt.y) * (i_mid->pt.y - m_msllHookCurrent.pt.y);
			if (m_buttonPressed && !m_dragging && m_setting->m_dragThreshold &&
				(m_setting->m_dragThreshold * m_setting->m_dragThreshold < dr)) {
				kid.MakeCode = 0;
				WaitForSingleObject(m_queueMutex, INFINITE);
				m_dragging = true;
				m_inputQueue->push_back(kid);
				SetEvent(m_readEvent);
				ReleaseMutex(m_queueMutex);
			}

			switch (g_hookData->m_mouseHookType) {
			case MouseHookType_Wheel:
				// For this type, g_hookData->m_mouseHookParam means
				// translate rate mouse move to wheel.
				mouse_event(MOUSEEVENTF_WHEEL, 0, 0,
							g_hookData->m_mouseHookParam * dy, 0);
				return 1;
				break;
			case MouseHookType_WindowMove: {
				RECT curRect;

				if (!GetWindowRect(target, &curRect))
					return 0;

				// g_hookData->m_mouseHookParam < 0 means
				// target window to move is MDI.
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
				g_hookData->m_mousePos = i_mid->pt;
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

		WaitForSingleObject(m_queueMutex, INFINITE);

		if (kid.Flags & KEYBOARD_INPUT_DATA::BREAK) {
			m_buttonPressed = false;
			if (m_dragging) {
				KEYBOARD_INPUT_DATA kid2;

				m_dragging = false;
				kid2.UnitId = 0;
				kid2.Flags = KEYBOARD_INPUT_DATA::E1 | KEYBOARD_INPUT_DATA::BREAK;
				kid2.Reserved = 0;
				kid2.ExtraInformation = 0;
				kid2.MakeCode = 0;
				m_inputQueue->push_back(kid2);
			}
		} else if (i_message != WM_MOUSEWHEEL && i_message != WM_MOUSEHWHEEL) {
			m_buttonPressed = true;
			m_msllHookCurrent = *i_mid;
		}

		m_inputQueue->push_back(kid);

		if (i_message == WM_MOUSEWHEEL || i_message == WM_MOUSEHWHEEL) {
			kid.UnitId = 0;
			kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
			kid.Reserved = 0;
			kid.ExtraInformation = 0;
			m_inputQueue->push_back(kid);
		}

		SetEvent(m_readEvent);
		ReleaseMutex(m_queueMutex);

		return 1;
	}
}
