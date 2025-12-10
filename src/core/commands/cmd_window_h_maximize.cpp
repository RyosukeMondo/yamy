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
    std::string sName = getName();
    const char* cName = sName.c_str();

    if (!i_sl->getOpenParen(false, cName))
      return;
    if (i_sl->getCloseParen(false, cName))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, cName); // throw ...
}

void Command_WindowHMaximize::exec(Engine *i_engine, FunctionParam *i_param) const
{
    Command_WindowHVMaximize cmd;
    cmd.m_isHorizontal = BooleanType_true;
    cmd.m_twt = m_twt;
    cmd.exec(i_engine, i_param);
}

tostream &Command_WindowHMaximize::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
