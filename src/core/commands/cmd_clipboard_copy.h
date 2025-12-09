#pragma once
#ifndef _CMD_CLIPBOARD_COPY_H
#define _CMD_CLIPBOARD_COPY_H

#include "command_base.h"

class Command_ClipboardCopy : public Command<Command_ClipboardCopy>
{
public:
    static constexpr const char *Name = "ClipboardCopy";

    StrExprArg m_text;

    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_CLIPBOARD_COPY_H
