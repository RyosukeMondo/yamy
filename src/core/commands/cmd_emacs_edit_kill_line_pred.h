#pragma once
#ifndef _CMD_EMACS_EDIT_KILL_LINE_PRED_H
#define _CMD_EMACS_EDIT_KILL_LINE_PRED_H

#include "command_base.h"

class Command_EmacsEditKillLinePred : public Command<Command_EmacsEditKillLinePred>
{
public:
    static constexpr const char *Name = "EmacsEditKillLinePred";

    const KeySeq * m_keySeq1;
    const KeySeq * m_keySeq2;

    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_EMACS_EDIT_KILL_LINE_PRED_H
