#pragma once
#ifndef _CMD_WINDOW_CLOSE_H
#define _CMD_WINDOW_CLOSE_H

#include "command_base.h"

class Command_WindowClose : public Command<Command_WindowClose>
{
public:
    static constexpr const _TCHAR *Name = _T("WindowClose");

    TargetWindowType m_twt;

    Command_WindowClose();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_WINDOW_CLOSE_H
