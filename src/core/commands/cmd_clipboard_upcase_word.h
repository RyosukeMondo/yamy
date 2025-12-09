#pragma once
#ifndef _CMD_CLIPBOARD_UPCASE_WORD_H
#define _CMD_CLIPBOARD_UPCASE_WORD_H

#include "command_base.h"

class Command_ClipboardUpcaseWord : public Command<Command_ClipboardUpcaseWord>
{
public:
    static constexpr const _TCHAR *Name = _T("ClipboardUpcaseWord");

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_CLIPBOARD_UPCASE_WORD_H
