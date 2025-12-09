#pragma once
#ifndef _CMD_WINDOW_LOWER_H
#define _CMD_WINDOW_LOWER_H

#include "command_base.h"

class Command_WindowLower : public Command<Command_WindowLower>
{
public:
    static constexpr const char *Name = "WindowLower";

    TargetWindowType m_twt;

    Command_WindowLower();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_WINDOW_LOWER_H
