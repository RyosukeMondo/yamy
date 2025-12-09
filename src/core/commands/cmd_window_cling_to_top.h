#pragma once
#ifndef _CMD_WINDOW_CLING_TO_TOP_H
#define _CMD_WINDOW_CLING_TO_TOP_H

#include "command_base.h"

class Command_WindowClingToTop : public Command<Command_WindowClingToTop>
{
public:
    static constexpr const _TCHAR *Name = _T("WindowClingToTop");

    TargetWindowType m_twt;

    Command_WindowClingToTop();
    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_WINDOW_CLING_TO_TOP_H
