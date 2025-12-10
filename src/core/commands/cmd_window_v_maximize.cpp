#include "cmd_window_v_maximize.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "cmd_window_hv_maximize.h" // Reuse logic

Command_WindowVMaximize::Command_WindowVMaximize()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowVMaximize::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    if (!i_sl->getOpenParen(false, tName))
      return;
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_WindowVMaximize::exec(Engine *i_engine, FunctionParam *i_param) const
{
    Command_WindowHVMaximize cmd;
    cmd.m_isHorizontal = BooleanType_false;
    cmd.m_twt = m_twt;
    cmd.exec(i_engine, i_param);
}

tostream &Command_WindowVMaximize::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
