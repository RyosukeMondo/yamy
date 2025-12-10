#pragma once
#ifndef _CMD_WINDOW_MOVE_TO_H
#define _CMD_WINDOW_MOVE_TO_H

#include "command_base.h"

class Command_WindowMoveTo : public Command<Command_WindowMoveTo>
{
public:
    static constexpr const char *Name = "WindowMoveTo";

    GravityType m_gravityType;
    int m_dx;
    int m_dy;
    TargetWindowType m_twt;

    Command_WindowMoveTo();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual std::ostream &outputArgs(std::ostream &i_ost) const override;
};

#endif // _CMD_WINDOW_MOVE_TO_H
