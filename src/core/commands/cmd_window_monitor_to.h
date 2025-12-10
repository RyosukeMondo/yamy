#pragma once
#ifndef _CMD_WINDOW_MONITOR_TO_H
#define _CMD_WINDOW_MONITOR_TO_H

#include "command_base.h"

class Command_WindowMonitorTo : public Command<Command_WindowMonitorTo>
{
public:
    static constexpr const char *Name = "WindowMonitorTo";

    WindowMonitorFromType m_fromType;
    int m_monitor;
    BooleanType m_adjustPos;
    BooleanType m_adjustSize;

    Command_WindowMonitorTo();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual std::ostream &outputArgs(std::ostream &i_ost) const override;
};

#endif // _CMD_WINDOW_MONITOR_TO_H
