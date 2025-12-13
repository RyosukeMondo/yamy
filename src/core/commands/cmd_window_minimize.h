#pragma once
#ifndef _CMD_WINDOW_MINIMIZE_H
#define _CMD_WINDOW_MINIMIZE_H

#include "command_base.h"

class Command_WindowMinimize : public Command<Command_WindowMinimize>
{
public:
    static constexpr const char *Name = "WindowMinimize";

    TargetWindowType m_twt;

    Command_WindowMinimize();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual std::ostream &outputArgs(std::ostream &i_ost) const override;
};

#endif // _CMD_WINDOW_MINIMIZE_H
