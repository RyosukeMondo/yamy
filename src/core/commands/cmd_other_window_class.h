#pragma once
#ifndef _CMD_OTHER_WINDOW_CLASS_H
#define _CMD_OTHER_WINDOW_CLASS_H

#include "command_base.h"

class Command_OtherWindowClass : public Command<Command_OtherWindowClass>
{
public:
    static constexpr const char *Name = "OtherWindowClass";

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_OTHER_WINDOW_CLASS_H
