#pragma once
#ifndef _CMD_WAIT_H
#define _CMD_WAIT_H

#include "command_base.h"

class Command_Wait : public Command<Command_Wait, int>
{
public:
    static constexpr const _TCHAR *Name = _T("Wait");

    Command_Wait() = default;

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_WAIT_H
