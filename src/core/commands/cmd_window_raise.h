#pragma once
#ifndef _CMD_WINDOW_RAISE_H
#define _CMD_WINDOW_RAISE_H

#include "command_base.h"

class Command_WindowRaise : public Command<Command_WindowRaise>
{
public:
    static constexpr const char *Name = "WindowRaise";

    TargetWindowType m_twt;

    Command_WindowRaise();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_WINDOW_RAISE_H
