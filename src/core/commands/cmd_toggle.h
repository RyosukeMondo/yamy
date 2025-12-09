#pragma once
#ifndef _CMD_TOGGLE_H
#define _CMD_TOGGLE_H

#include "command_base.h"
#include "../functions/function.h" // ModifierLockType, ToggleType

class Command_Toggle : public Command<Command_Toggle, ModifierLockType, ToggleType>
{
public:
    static constexpr const char *Name = "Toggle";

    // Constructor to set default arguments
    Command_Toggle()
    {
        // Default: ToggleType = ToggleType_toggle
        std::get<1>(m_args) = ToggleType_toggle;
    }

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_TOGGLE_H
