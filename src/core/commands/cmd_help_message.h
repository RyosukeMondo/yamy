#pragma once
#ifndef _CMD_HELP_MESSAGE_H
#define _CMD_HELP_MESSAGE_H

#include "command_base.h"

class Command_HelpMessage : public Command<Command_HelpMessage>
{
public:
    static constexpr const char *Name = "HelpMessage";

    StrExprArg m_title;
    StrExprArg m_message;

    Command_HelpMessage();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_HELP_MESSAGE_H
