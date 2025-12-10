#pragma once
#ifndef _CMD_SET_FOREGROUND_WINDOW_H
#define _CMD_SET_FOREGROUND_WINDOW_H

#include "command_base.h"
#include "../functions/strexpr.h"
#include <ostream>

class Command_SetForegroundWindow : public Command<Command_SetForegroundWindow>
{
public:
    static constexpr const char *Name = "SetForegroundWindow";

    StrExprArg m_className;
    StrExprArg m_titleName;
    LogicalOperatorType m_logicalOp;

    Command_SetForegroundWindow();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual std::ostream &outputArgs(std::ostream &i_ost) const override;
};

#endif // _CMD_SET_FOREGROUND_WINDOW_H
