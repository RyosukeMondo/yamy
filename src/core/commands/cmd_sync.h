#pragma once
#ifndef _CMD_SYNC_H
#define _CMD_SYNC_H

#include "command_base.h"

class Command_Sync : public Command<Command_Sync>
{
public:
    static constexpr const char *Name = "Sync";

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_SYNC_H
