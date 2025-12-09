#pragma once
#ifndef _CMD_PREFIX_H
#define _CMD_PREFIX_H

#include "command_base.h"
#include "../functions/function.h" // BooleanType

class Command_Prefix : public Command<Command_Prefix, const Keymap*, BooleanType>
{
public:
    static constexpr const char *Name = "Prefix";

    // Constructor to set default arguments
    Command_Prefix()
    {
        // Default: doesIgnoreModifiers = true
        std::get<1>(m_args) = BooleanType_true;
    }

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_PREFIX_H
