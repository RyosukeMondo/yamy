#pragma once
#ifndef _CMD_MAYU_DIALOG_H
#define _CMD_MAYU_DIALOG_H

#include "command_base.h"

class Command_MayuDialog : public Command<Command_MayuDialog>
{
public:
    static constexpr const char *Name = "MayuDialog";

    MayuDialogType m_dialog;
    ShowCommandType m_showCommand;

    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_MAYU_DIALOG_H
