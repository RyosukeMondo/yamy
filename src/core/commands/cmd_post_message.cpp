#include "cmd_post_message.h"
#include "../engine/engine.h"
#include "../functions/function.h"

void Command_PostMessage::exec(Engine *i_engine, FunctionParam *i_param) const
{
	// Logic reconstructed from Engine::funcPostMessage
	ToWindowType i_window = std::get<0>(m_args);
	UINT i_message = std::get<1>(m_args);
	WPARAM i_wParam = std::get<2>(m_args);
	LPARAM i_lParam = std::get<3>(m_args);

	if (!i_param->m_isPressed)
		return;

	int window = static_cast<int>(i_window);
	HWND hwnd = i_param->m_hwnd;

	if (0 < window) {
		for (int i = 0; i < window; ++ i)
			hwnd = (HWND)i_engine->m_windowSystem->getParent((WindowSystem::WindowHandle)hwnd);
	} else if (window == ToWindowType_toMainWindow) {
		while (true) {
			HWND p = (HWND)i_engine->m_windowSystem->getParent((WindowSystem::WindowHandle)hwnd);
			if (!p)
				break;
			hwnd = p;
		}
	} else if (window == ToWindowType_toOverlappedWindow) {
		while (hwnd) {
			if (!i_engine->m_windowSystem->isChild((WindowSystem::WindowHandle)hwnd))
				break;
			hwnd = (HWND)i_engine->m_windowSystem->getParent((WindowSystem::WindowHandle)hwnd);
		}
	}

	if (hwnd)
		i_engine->m_windowSystem->postMessage((WindowSystem::WindowHandle)hwnd, i_message, (uintptr_t)i_wParam, (intptr_t)i_lParam);
}
