#pragma once
#ifndef _CMD_WINDOW_CLING_TO_LEFT_H
#define _CMD_WINDOW_CLING_TO_LEFT_H

#include "command_base.h"

class Command_WindowClingToLeft : public Command<Command_WindowClingToLeft>
{
public:
    static constexpr const char *Name = "WindowClingToLeft";

    TargetWindowType m_twt;

    Command_WindowClingToLeft();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual std::ostream &outputArgs(std::ostream &i_ost) const override;
};

#endif // _CMD_WINDOW_CLING_TO_LEFT_H
