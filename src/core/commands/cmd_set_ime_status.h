#pragma once
#ifndef _CMD_SET_IME_STATUS_H
#define _CMD_SET_IME_STATUS_H

#include "command_base.h"

class Command_SetImeStatus : public Command<Command_SetImeStatus>
{
public:
    static constexpr const char *Name = "SetImeStatus";

    ToggleType m_toggle;

    Command_SetImeStatus();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual std::ostream &outputArgs(std::ostream &i_ost) const override;
};

#endif // _CMD_SET_IME_STATUS_H
