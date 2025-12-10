#include "cmd_window_identify.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../../platform/windows/windowstool.h" // For getToplevelWindow
#include "../../platform/windows/hook.h" // For WM_MAYU_MESSAGE_NAME

void Command_WindowIdentify::load(SettingLoader *i_sl)
{
    // no argument
}

void Command_WindowIdentify::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (i_param->m_isPressed) {
        tstring className = to_tstring(i_engine->getWindowSystem()->getClassName(i_param->m_hwnd));
        tstring titleName = to_tstring(i_engine->getWindowSystem()->getTitleName(i_param->m_hwnd));

        Acquire a(&i_engine->m_log, 1);
        i_engine->m_log << "WindowHandle:\t0x" << std::hex << (uintptr_t)i_param->m_hwnd << std::dec << std::endl;
        i_engine->m_log << _T("CLASS:\t") << className << std::endl;
        i_engine->m_log << _T("TITLE:\t") << titleName << std::endl;

        tstring msgName = addSessionId(WM_MAYU_MESSAGE_NAME);
        unsigned int message = i_engine->getWindowSystem()->registerWindowMessage(to_string(msgName));
        uintptr_t result;
        if (i_engine->getWindowSystem()->sendMessageTimeout(
                    i_param->m_hwnd, message, 0, 0,
                    0x0002, 100, &result)) { // SMTO_ABORTIFHUNG
            i_engine->m_log << _T("mayu message is processed.") << std::endl;
        }
    }
}

tostream &Command_WindowIdentify::outputArgs(tostream &i_ost) const
{
    return i_ost;
}
