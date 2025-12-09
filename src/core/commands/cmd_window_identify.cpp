#include "cmd_window_identify.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../utils/misc.h" // for NUMBER_OF
#include "../../platform/windows/windowstool.h" // For getToplevelWindow

void Command_WindowIdentify::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    tstring className = i_engine->m_windowSystem->getClassName((WindowSystem::WindowHandle)i_param->m_hwnd);
    bool ok = false;
    if (!className.empty()) {
        if (_tcsicmp(className.c_str(), _T("ConsoleWindowClass")) == 0) {
            tstring titleName = i_engine->m_windowSystem->getTitleName((WindowSystem::WindowHandle)i_param->m_hwnd);
            {
                Acquire a(&i_engine->m_log, 1);
                i_engine->m_log << _T("HWND:\t") << std::hex
                << reinterpret_cast<ULONG_PTR>(i_param->m_hwnd)
                << std::dec << std::endl;
            }
            Acquire a(&i_engine->m_log, 0);
            i_engine->m_log << _T("CLASS:\t") << className << std::endl;
            i_engine->m_log << _T("TITLE:\t") << titleName << std::endl;

            HWND hwnd = getToplevelWindow(i_param->m_hwnd, nullptr);
            WindowRect rc;
            i_engine->m_windowSystem->getWindowRect((WindowSystem::WindowHandle)hwnd, &rc);
            i_engine->m_log << _T("Toplevel Window Position/Size: (")
            << rc.left << _T(", ") << rc.top << _T(") / (")
            << (rc.right - rc.left) << _T("x") << (rc.bottom - rc.top)
            << _T(")") << std::endl;

            i_engine->m_windowSystem->getWorkArea(&rc);
            i_engine->m_log << _T("Desktop Window Position/Size: (")
            << rc.left << _T(", ") << rc.top << _T(") / (")
            << (rc.right - rc.left) << _T("x") << (rc.bottom - rc.top)
            << _T(")") << std::endl;

            i_engine->m_log << std::endl;
            ok = true;
        }
    }
    if (!ok) {
        UINT WM_MAYU_MESSAGE = i_engine->m_windowSystem->registerWindowMessage(
                                   addSessionId(WM_MAYU_MESSAGE_NAME).c_str());
        CHECK_TRUE( i_engine->m_windowSystem->postMessage((WindowSystem::WindowHandle)i_param->m_hwnd, WM_MAYU_MESSAGE,
                                MayuMessage_notifyName, 0) );
    }
}
