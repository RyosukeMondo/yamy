#pragma once
#ifndef _CMD_KEYMAP_PREV_PREFIX_H
#define _CMD_KEYMAP_PREV_PREFIX_H

#include "command_base.h"

class Command_KeymapPrevPrefix : public Command<Command_KeymapPrevPrefix, int>
{
public:
    static constexpr const _TCHAR *Name = _T("KeymapPrevPrefix");

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override
    {
        i_engine->funcKeymapPrevPrefix(i_param, getArg<0>());
    }
};

#endif // _CMD_KEYMAP_PREV_PREFIX_H
