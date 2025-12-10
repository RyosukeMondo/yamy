#pragma once
#ifndef _CMD_WINDOW_CLOSE_H
#define _CMD_WINDOW_CLOSE_H

#include "command_base.h"

class Command_WindowClose : public Command<Command_WindowClose>
{
public:
    static constexpr const char *Name = "WindowClose";

    TargetWindowType m_twt;

    Command_WindowClose();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual std::ostream &outputArgs(std::ostream &i_ost) const override;
};

#endif // _CMD_WINDOW_CLOSE_H
