#pragma once
#ifndef _CMD_REPEAT_H
#define _CMD_REPEAT_H

#include "command_base.h"

#include "../input/keymap.h"

class Command_Repeat : public Command<Command_Repeat, const KeySeq*, int>
{
public:
    static constexpr const _TCHAR *Name = _T("Repeat");

    Command_Repeat() {
        std::get<1>(m_args) = 10; // Default max = 10
        std::get<0>(m_args) = NULL;
    }
    // Default copy constructor and destructor are sufficient as we don't own KeySeq
    // Command_Repeat(const Command_Repeat &i_o);
    // virtual ~Command_Repeat();

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_REPEAT_H
