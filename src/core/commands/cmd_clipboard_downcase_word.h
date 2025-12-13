#pragma once
#ifndef _CMD_CLIPBOARD_DOWNCASE_WORD_H
#define _CMD_CLIPBOARD_DOWNCASE_WORD_H

#include "command_base.h"

class Command_ClipboardDowncaseWord : public Command<Command_ClipboardDowncaseWord>
{
public:
    static constexpr const char *Name = "ClipboardDowncaseWord";

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_CLIPBOARD_DOWNCASE_WORD_H
