#include "cmd_help_variable.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../utils/misc.h" // for NUMBER_OF

void Command_HelpVariable::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_title);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_HelpVariable::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    char buf[20];
    snprintf(buf, NUMBER_OF(buf), "%d", i_engine->m_variable);

    i_engine->m_helpTitle = m_title.eval();
    i_engine->m_helpMessage = buf;
    i_engine->getWindowSystem()->postMessage(i_engine->getAssociatedWndow(), WM_APP_engineNotify,
                EngineNotify_helpMessage, true);
}

std::ostream &Command_HelpVariable::outputArgs(std::ostream &i_ost) const
{
    i_ost << m_title;
    return i_ost;
}
