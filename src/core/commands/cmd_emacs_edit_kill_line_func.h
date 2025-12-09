#pragma once
#ifndef _CMD_EMACS_EDIT_KILL_LINE_FUNC_H
#define _CMD_EMACS_EDIT_KILL_LINE_FUNC_H

#include "command_base.h"

class Command_EmacsEditKillLineFunc : public Command<Command_EmacsEditKillLineFunc>
{
public:
    static constexpr const char *Name = "EmacsEditKillLineFunc";

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_EMACS_EDIT_KILL_LINE_FUNC_H
