#pragma once
#ifndef _CMD_DEFAULT_H
#define _CMD_DEFAULT_H

#include "command_base.h"

class Command_Default : public Command<Command_Default>
{
public:
    static constexpr const char *Name = "Default";

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_DEFAULT_H
