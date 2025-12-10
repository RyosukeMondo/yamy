#include "cmd_set_ime_string.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

#ifndef WM_MAYU_MESSAGE_NAME
#define WM_MAYU_MESSAGE_NAME "YAMY_MAYU_MESSAGE"
#endif
#ifndef MayuMessage_funcSetImeString
#define MayuMessage_funcSetImeString 3
#endif

void Command_SetImeString::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_data);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_SetImeString::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;
    if (i_engine->m_hwndFocus) {
        UINT WM_MAYU_MESSAGE = i_engine->getWindowSystem()->registerWindowMessage(
                                   addSessionId(WM_MAYU_MESSAGE_NAME));
        i_engine->getWindowSystem()->postMessage(i_engine->m_hwndFocus, WM_MAYU_MESSAGE, MayuMessage_funcSetImeString, m_data.eval().size() * sizeof(char));

        unsigned int len = 0;
        i_engine->getWindowSystem()->disconnectNamedPipe(i_engine->m_hookPipe);
        i_engine->getWindowSystem()->connectNamedPipe(i_engine->m_hookPipe, nullptr);
        i_engine->getWindowSystem()->writeFile(i_engine->m_hookPipe, m_data.eval().c_str(),
                          (unsigned int)(m_data.eval().size() * sizeof(char)),
                          &len, nullptr);
    }
}

std::ostream &Command_SetImeString::outputArgs(std::ostream &i_ost) const
{
    i_ost << m_data;
    return i_ost;
}
