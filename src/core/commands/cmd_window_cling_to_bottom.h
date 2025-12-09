#pragma once
#ifndef _CMD_WINDOW_CLING_TO_BOTTOM_H
#define _CMD_WINDOW_CLING_TO_BOTTOM_H

#include "command_base.h"

class Command_WindowClingToBottom : public Command<Command_WindowClingToBottom>
{
public:
    static constexpr const char *Name = "WindowClingToBottom";

    TargetWindowType m_twt;

    Command_WindowClingToBottom();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_WINDOW_CLING_TO_BOTTOM_H
