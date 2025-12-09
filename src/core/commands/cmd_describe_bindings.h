#pragma once
#ifndef _CMD_DESCRIBE_BINDINGS_H
#define _CMD_DESCRIBE_BINDINGS_H

#include "command_base.h"

class Command_DescribeBindings : public Command<Command_DescribeBindings>
{
public:
    static constexpr const char *Name = "DescribeBindings";

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_DESCRIBE_BINDINGS_H
