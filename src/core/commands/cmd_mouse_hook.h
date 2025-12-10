#pragma once
#ifndef _CMD_MOUSE_HOOK_H
#define _CMD_MOUSE_HOOK_H

#include "command_base.h"

class Command_MouseHook : public Command<Command_MouseHook>
{
public:
    static constexpr const char *Name = "MouseHook";

    MouseHookType m_hookType;
    int m_hookParam;

    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual std::ostream &outputArgs(std::ostream &i_ost) const override;
};

#endif // _CMD_MOUSE_HOOK_H
