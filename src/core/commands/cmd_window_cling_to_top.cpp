#include "cmd_window_cling_to_top.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "cmd_window_move_to.h" // Reuse logic

Command_WindowClingToTop::Command_WindowClingToTop()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowClingToTop::load(SettingLoader *i_sl)
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

void Command_WindowClingToTop::exec(Engine *i_engine, FunctionParam *i_param) const
{
    Command_WindowMoveTo cmd;
    cmd.m_gravityType = GravityType_N;
    cmd.m_dx = 0;
    cmd.m_dy = 0;
    cmd.m_twt = m_twt;
    cmd.exec(i_engine, i_param);
}

tostream &Command_WindowClingToTop::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
