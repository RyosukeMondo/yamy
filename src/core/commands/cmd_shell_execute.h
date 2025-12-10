#pragma once
#ifndef _CMD_SHELL_EXECUTE_H
#define _CMD_SHELL_EXECUTE_H

#include "command_base.h"

class Command_ShellExecute : public Command<Command_ShellExecute>
{
public:
    static constexpr const char *Name = "ShellExecute";

    StrExprArg m_operation;
    StrExprArg m_file;
    StrExprArg m_parameters;
    StrExprArg m_directory;
    ShowCommandType m_showCommand;

    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual std::ostream &output(std::ostream &i_ost) const override;

    // Helper to execute logic on main thread
    static void executeOnMainThread(Engine *i_engine);
};

#endif // _CMD_SHELL_EXECUTE_H
