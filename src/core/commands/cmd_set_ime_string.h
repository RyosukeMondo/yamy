#pragma once
#ifndef _CMD_SET_IME_STRING_H
#define _CMD_SET_IME_STRING_H

#include "command_base.h"

class Command_SetImeString : public Command<Command_SetImeString>
{
public:
    static constexpr const char *Name = "SetImeString";

    StrExprArg m_data;

    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_SET_IME_STRING_H
