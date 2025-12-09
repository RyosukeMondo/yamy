#pragma once
#ifndef _CMD_LOG_CLEAR_H
#define _CMD_LOG_CLEAR_H

#include "command_base.h"

class Command_LogClear : public Command<Command_LogClear>
{
public:
    static constexpr const _TCHAR *Name = _T("LogClear");

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_LOG_CLEAR_H
