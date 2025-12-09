#pragma once
#ifndef _CMD_PLUGIN_H
#define _CMD_PLUGIN_H

#include "command_base.h"

class Command_PlugIn : public Command<Command_PlugIn>
{
public:
    static constexpr const char *Name = "PlugIn";

    StrExprArg m_dllName;
    StrExprArg m_funcName;
    StrExprArg m_funcParam;
    BooleanType m_doesCreateThread;

    Command_PlugIn();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_PLUGIN_H
