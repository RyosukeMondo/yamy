#pragma once
#ifndef _CMD_WINDOW_REDRAW_H
#define _CMD_WINDOW_REDRAW_H

#include "command_base.h"

class Command_WindowRedraw : public Command<Command_WindowRedraw>
{
public:
    static constexpr const char *Name = "WindowRedraw";

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_WINDOW_REDRAW_H
