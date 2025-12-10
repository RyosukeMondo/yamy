#include "cmd_help_message.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_HelpMessage::Command_HelpMessage()
{
    m_title = StrExprArg();
    m_message = StrExprArg();
}

void Command_HelpMessage::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    if (!i_sl->getOpenParen(false, tName))
      return;
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->load_ARGUMENT(&m_title);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_message);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_HelpMessage::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    i_engine->m_helpTitle = m_title.eval();
    i_engine->m_helpMessage = m_message.eval();
    bool doesShow = !(m_title.eval().size() == 0 && m_message.eval().size() == 0);
    i_engine->getWindowSystem()->postMessage(i_engine->getAssociatedWndow(), WM_APP_engineNotify,
                EngineNotify_helpMessage, doesShow);
}

std::ostream &Command_HelpMessage::outputArgs(std::ostream &i_ost) const
{
    i_ost << m_title << ", ";
    i_ost << m_message;
    return i_ost;
}
