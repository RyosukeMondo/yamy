#pragma once
#ifndef _CMD_WINDOW_MAXIMIZE_H
#define _CMD_WINDOW_MAXIMIZE_H

#include "command_base.h"

class Command_WindowMaximize : public Command<Command_WindowMaximize>
{
public:
    static constexpr const char *Name = "WindowMaximize";

    TargetWindowType m_twt;

    Command_WindowMaximize();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_WINDOW_MAXIMIZE_H
