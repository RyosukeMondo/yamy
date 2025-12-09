#pragma once
#ifndef _CMD_WINDOW_MONITOR_H
#define _CMD_WINDOW_MONITOR_H

#include "command_base.h"

class Command_WindowMonitor : public Command<Command_WindowMonitor>
{
public:
    static constexpr const _TCHAR *Name = _T("WindowMonitor");

    int m_monitor;
    BooleanType m_adjustPos;
    BooleanType m_adjustSize;

    Command_WindowMonitor();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_WINDOW_MONITOR_H
