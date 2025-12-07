#ifndef _CMD_EDIT_NEXT_MODIFIER_H
#define _CMD_EDIT_NEXT_MODIFIER_H

#include "command_base.h"
#include "../input/keyboard.h"

class Command_EditNextModifier : public Command<Command_EditNextModifier, Modifier>
{
public:
    static constexpr const _TCHAR *Name = _T("EditNextModifier");

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_EDIT_NEXT_MODIFIER_H
