#pragma once
#ifndef _CMD_INVESTIGATE_COMMAND_H
#define _CMD_INVESTIGATE_COMMAND_H

#include "command_base.h"

class Command_InvestigateCommand : public Command<Command_InvestigateCommand>
{
public:
    static constexpr const char *Name = "InvestigateCommand";

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_INVESTIGATE_COMMAND_H
