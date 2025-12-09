#pragma once
#ifndef _CMD_SET_FOREGROUND_WINDOW_H
#define _CMD_SET_FOREGROUND_WINDOW_H

#include "command_base.h"

class Command_SetForegroundWindow : public Command<Command_SetForegroundWindow>
{
public:
    static constexpr const char *Name = "SetForegroundWindow";

    tregex m_windowClassName;
    LogicalOperatorType m_logicalOp;
    tregex m_windowTitleName;

    Command_SetForegroundWindow();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_SET_FOREGROUND_WINDOW_H
