#pragma once
#ifndef _CMD_WINDOW_HV_MAXIMIZE_H
#define _CMD_WINDOW_HV_MAXIMIZE_H

#include "command_base.h"

class Command_WindowHVMaximize : public Command<Command_WindowHVMaximize>
{
public:
    static constexpr const _TCHAR *Name = _T("WindowHVMaximize");

    BooleanType m_isHorizontal;
    TargetWindowType m_twt;

    Command_WindowHVMaximize();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_WINDOW_HV_MAXIMIZE_H
