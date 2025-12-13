#pragma once
#ifndef _CMD_WINDOW_MOVE_VISIBLY_H
#define _CMD_WINDOW_MOVE_VISIBLY_H

#include "command_base.h"

class Command_WindowMoveVisibly : public Command<Command_WindowMoveVisibly>
{
public:
    static constexpr const char *Name = "WindowMoveVisibly";

    TargetWindowType m_twt;

    Command_WindowMoveVisibly();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual std::ostream &outputArgs(std::ostream &i_ost) const override;
};

#endif // _CMD_WINDOW_MOVE_VISIBLY_H
