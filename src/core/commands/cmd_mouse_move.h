#pragma once
#ifndef _CMD_MOUSE_MOVE_H
#define _CMD_MOUSE_MOVE_H

#include "command_base.h"

class Command_MouseMove : public Command<Command_MouseMove>
{
public:
    static constexpr const _TCHAR *Name = _T("MouseMove");

    int m_dx;
    int m_dy;

    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_MOUSE_MOVE_H
