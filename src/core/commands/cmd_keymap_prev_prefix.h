#pragma once
#ifndef _CMD_KEYMAP_PREV_PREFIX_H
#define _CMD_KEYMAP_PREV_PREFIX_H

#include "command_base.h"

class Command_KeymapPrevPrefix : public Command<Command_KeymapPrevPrefix, int>
{
public:
    static constexpr const char *Name = "KeymapPrevPrefix";

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_KEYMAP_PREV_PREFIX_H
