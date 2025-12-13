#pragma once
#ifndef _CMD_CANCEL_PREFIX_H
#define _CMD_CANCEL_PREFIX_H

#include "command_base.h"

class Command_CancelPrefix : public Command<Command_CancelPrefix>
{
public:
    static constexpr const char *Name = "CancelPrefix";

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_CANCEL_PREFIX_H
