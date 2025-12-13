#include "cmd_window_monitor.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "cmd_window_monitor_to.h" // Reuse logic

Command_WindowMonitor::Command_WindowMonitor()
{
    m_adjustPos = BooleanType_true;
    m_adjustSize = BooleanType_false;
}

void Command_WindowMonitor::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_monitor);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_adjustPos);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_adjustSize);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_WindowMonitor::exec(Engine *i_engine, FunctionParam *i_param) const
{
    Command_WindowMonitorTo cmd;
    cmd.m_fromType = WindowMonitorFromType_primary;
    cmd.m_monitor = m_monitor;
    cmd.m_adjustPos = m_adjustPos;
    cmd.m_adjustSize = m_adjustSize;
    cmd.exec(i_engine, i_param);
}

std::ostream &Command_WindowMonitor::outputArgs(std::ostream &i_ost) const
{
    i_ost << m_monitor << ", ";
    i_ost << m_adjustPos << ", ";
    i_ost << m_adjustSize;
    return i_ost;
}
