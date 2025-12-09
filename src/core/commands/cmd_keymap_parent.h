#pragma once
#ifndef _CMD_KEYMAP_PARENT_H
#define _CMD_KEYMAP_PARENT_H

#include "command_base.h"

class Command_KeymapParent : public Command<Command_KeymapParent>
{
public:
    static constexpr const char *Name = "KeymapParent";

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_KEYMAP_PARENT_H
