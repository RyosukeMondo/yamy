#pragma once
#ifndef _CMD_WINDOW_SET_ALPHA_H
#define _CMD_WINDOW_SET_ALPHA_H

#include "command_base.h"

class Command_WindowSetAlpha : public Command<Command_WindowSetAlpha>
{
public:
    static constexpr const _TCHAR *Name = _T("WindowSetAlpha");

    int m_alpha;

    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_WINDOW_SET_ALPHA_H
