#pragma once
#ifndef _CMD_KEYMAP_WINDOW_H
#define _CMD_KEYMAP_WINDOW_H

#include "command_base.h"

class Command_KeymapWindow : public Command<Command_KeymapWindow>
{
public:
    static constexpr const _TCHAR *Name = _T("KeymapWindow");

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_KEYMAP_WINDOW_H
