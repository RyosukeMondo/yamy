#pragma once
#ifndef _CMD_HELP_VARIABLE_H
#define _CMD_HELP_VARIABLE_H

#include "command_base.h"

class Command_HelpVariable : public Command<Command_HelpVariable>
{
public:
    static constexpr const _TCHAR *Name = _T("HelpVariable");

    StrExprArg m_title;

    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_HELP_VARIABLE_H
