#pragma once
#ifndef _CMD_CLIPBOARD_CHANGE_CASE_H
#define _CMD_CLIPBOARD_CHANGE_CASE_H

#include "command_base.h"

class Command_ClipboardChangeCase : public Command<Command_ClipboardChangeCase>
{
public:
    static constexpr const char *Name = "ClipboardChangeCase";

    BooleanType m_doesConvertToUpperCase;

    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_CLIPBOARD_CHANGE_CASE_H
