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
    const char* tName = Name;

    if (!i_sl->getOpenParen(false, tName))
      return;
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, tName); // throw ...
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

std::ostream &Command_WindowClingToLeft::outputArgs(std::ostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
