#pragma once
#ifndef _CMD_WINDOW_IDENTIFY_H
#define _CMD_WINDOW_IDENTIFY_H

#include "command_base.h"

class Command_WindowIdentify : public Command<Command_WindowIdentify>
{
public:
    static constexpr const _TCHAR *Name = _T("WindowIdentify");

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_WINDOW_IDENTIFY_H
