#pragma once
#ifndef _CMD_WINDOW_RESIZE_TO_H
#define _CMD_WINDOW_RESIZE_TO_H

#include "command_base.h"

class Command_WindowResizeTo : public Command<Command_WindowResizeTo>
{
public:
    static constexpr const char *Name = "WindowResizeTo";

    int m_width;
    int m_height;
    TargetWindowType m_twt;

    Command_WindowResizeTo();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_WINDOW_RESIZE_TO_H
