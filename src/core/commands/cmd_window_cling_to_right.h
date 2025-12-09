#pragma once
#ifndef _CMD_WINDOW_CLING_TO_RIGHT_H
#define _CMD_WINDOW_CLING_TO_RIGHT_H

#include "command_base.h"

class Command_WindowClingToRight : public Command<Command_WindowClingToRight>
{
public:
    static constexpr const char *Name = "WindowClingToRight";

    TargetWindowType m_twt;

    Command_WindowClingToRight();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_WINDOW_CLING_TO_RIGHT_H
