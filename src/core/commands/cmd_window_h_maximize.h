#pragma once
#ifndef _CMD_WINDOW_H_MAXIMIZE_H
#define _CMD_WINDOW_H_MAXIMIZE_H

#include "command_base.h"

class Command_WindowHMaximize : public Command<Command_WindowHMaximize>
{
public:
    static constexpr const char *Name = "WindowHMaximize";

    TargetWindowType m_twt;

    Command_WindowHMaximize();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_WINDOW_H_MAXIMIZE_H
