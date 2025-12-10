#pragma once
#ifndef _CMD_POST_MESSAGE_H
#define _CMD_POST_MESSAGE_H

#include "command_base.h"
#include "../functions/function.h" // For ToWindowType
#include "../platform/types.h"

class Command_PostMessage : public Command<Command_PostMessage, ToWindowType, yamy::platform::MessageId, yamy::platform::MessageWParam, yamy::platform::MessageLParam>
{
public:
    static constexpr const char *Name = "PostMessage";

    Command_PostMessage() = default;

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_POST_MESSAGE_H
