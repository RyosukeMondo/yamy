#pragma once
#ifndef _CMD_MOUSE_WHEEL_H
#define _CMD_MOUSE_WHEEL_H

#include "command_base.h"

class Command_MouseWheel : public Command<Command_MouseWheel>
{
public:
    static constexpr const char *Name = "MouseWheel";

    int m_delta;

    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual std::ostream &outputArgs(std::ostream &i_ost) const override;
};

#endif // _CMD_MOUSE_WHEEL_H
