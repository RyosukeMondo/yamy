#include "cmd_set_ime_status.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_SetImeStatus::Command_SetImeStatus()
{
    m_toggle = ToggleType_toggle;
}

void Command_SetImeStatus::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    if (!i_sl->getOpenParen(false, tName))
      return;
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->load_ARGUMENT(&m_toggle);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_SetImeStatus::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;
    if (i_engine->m_hwndFocus) {
        UINT WM_MAYU_MESSAGE = i_engine->m_windowSystem->registerWindowMessage(
                                   addSessionId(WM_MAYU_MESSAGE_NAME).c_str());
        int status = -1;
        switch (m_toggle) {
        case ToggleType_toggle:
            status = -1;
            break;
        case ToggleType_off:
            status = 0;
            break;
        case ToggleType_on:
            status = 1;
            break;
        }
        i_engine->m_windowSystem->postMessage((WindowSystem::WindowHandle)i_engine->m_hwndFocus, WM_MAYU_MESSAGE, MayuMessage_funcSetImeStatus, status);
    }
}

tostream &Command_SetImeStatus::outputArgs(tostream &i_ost) const
{
    i_ost << m_toggle;
    return i_ost;
}
