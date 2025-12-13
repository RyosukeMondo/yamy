#include "cmd_window_h_maximize.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "cmd_window_hv_maximize.h" // Reuse logic

Command_WindowHMaximize::Command_WindowHMaximize()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowHMaximize::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    if (!i_sl->getOpenParen(false, tName))
      return;
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_WindowHMaximize::exec(Engine *i_engine, FunctionParam *i_param) const
{
    Command_WindowHVMaximize cmd;
    cmd.m_isHorizontal = BooleanType_true;
    cmd.m_twt = m_twt;
    cmd.exec(i_engine, i_param);
}

std::ostream &Command_WindowHMaximize::outputArgs(std::ostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
