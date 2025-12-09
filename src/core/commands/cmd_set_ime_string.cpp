#include "cmd_set_ime_string.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_SetImeString::load(SettingLoader *i_sl)
{
    i_sl->getOpenParen(true, Name); // throw ...
    i_sl->load_ARGUMENT(&m_data);
    i_sl->getCloseParen(true, Name); // throw ...
}

void Command_SetImeString::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;
    if (i_engine->m_hwndFocus) {
        UINT WM_MAYU_MESSAGE = i_engine->m_windowSystem->registerWindowMessage(
                                   addSessionId(WM_MAYU_MESSAGE_NAME).c_str());
        i_engine->m_windowSystem->postMessage((WindowSystem::WindowHandle)i_engine->m_hwndFocus, WM_MAYU_MESSAGE, MayuMessage_funcSetImeString, m_data.eval().size() * sizeof(_TCHAR));

        unsigned int len = 0;
        i_engine->m_windowSystem->disconnectNamedPipe(i_engine->m_hookPipe);
        i_engine->m_windowSystem->connectNamedPipe(i_engine->m_hookPipe, nullptr);
        i_engine->m_windowSystem->writeFile(i_engine->m_hookPipe, m_data.eval().c_str(),
                          (unsigned int)(m_data.eval().size() * sizeof(_TCHAR)),
                          &len, nullptr);
    }
}

tostream &Command_SetImeString::outputArgs(tostream &i_ost) const
{
    i_ost << m_data;
    return i_ost;
}
