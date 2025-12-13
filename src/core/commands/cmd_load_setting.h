#pragma once
#ifndef _CMD_LOAD_SETTING_H
#define _CMD_LOAD_SETTING_H

#include "command_base.h"

class Command_LoadSetting : public Command<Command_LoadSetting, StrExprArg>
{
public:
    static constexpr const char *Name = "LoadSetting";

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_LOAD_SETTING_H
