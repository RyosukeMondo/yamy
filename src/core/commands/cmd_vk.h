#pragma once
#ifndef _CMD_VK_H
#define _CMD_VK_H

#include "command_base.h"
#include "../input/keyboard.h" // Assuming VKey is here

class Command_VK : public Command<Command_VK, VKey>
{
public:
    static constexpr const _TCHAR *Name = _T("VK");

    Command_VK() = default;

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_VK_H
