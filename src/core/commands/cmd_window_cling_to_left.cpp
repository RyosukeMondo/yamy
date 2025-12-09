#include "cmd_window_cling_to_left.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "cmd_window_move_to.h" // Reuse logic

Command_WindowClingToLeft::Command_WindowClingToLeft()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowClingToLeft::load(SettingLoader *i_sl)
{
    if (!i_sl->getOpenParen(false, Name))
      return;
    if (i_sl->getCloseParen(false, Name))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, Name); // throw ...
}

void Command_WindowClingToLeft::exec(Engine *i_engine, FunctionParam *i_param) const
{
    Command_WindowMoveTo cmd;
    cmd.m_gravityType = GravityType_W;
    cmd.m_dx = 0;
    cmd.m_dy = 0;
    cmd.m_twt = m_twt;
    cmd.exec(i_engine, i_param);
}

tostream &Command_WindowClingToLeft::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
