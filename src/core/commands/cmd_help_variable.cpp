#include "cmd_help_variable.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../utils/misc.h" // for NUMBER_OF

void Command_HelpVariable::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_title);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_HelpVariable::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    _TCHAR buf[20];
    _sntprintf(buf, NUMBER_OF(buf), _T("%d"), i_engine->m_variable);

    i_engine->m_helpTitle = m_title.eval();
    i_engine->m_helpMessage = buf;
    i_engine->getWindowSystem()->postMessage(i_engine->getAssociatedWndow(), WM_APP_engineNotify,
                EngineNotify_helpMessage, true);
}

tostream &Command_HelpVariable::outputArgs(tostream &i_ost) const
{
    i_ost << m_title;
    return i_ost;
}
