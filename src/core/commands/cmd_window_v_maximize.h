#pragma once
#ifndef _CMD_WINDOW_V_MAXIMIZE_H
#define _CMD_WINDOW_V_MAXIMIZE_H

#include "command_base.h"

class Command_WindowVMaximize : public Command<Command_WindowVMaximize>
{
public:
    static constexpr const _TCHAR *Name = _T("WindowVMaximize");

    TargetWindowType m_twt;

    Command_WindowVMaximize();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_WINDOW_V_MAXIMIZE_H
