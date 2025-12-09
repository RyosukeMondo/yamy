#pragma once
#ifndef _CMD_RECENTER_H
#define _CMD_RECENTER_H

#include "command_base.h"

class Command_Recenter : public Command<Command_Recenter>
{
public:
    static constexpr const _TCHAR *Name = _T("Recenter");

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_RECENTER_H
