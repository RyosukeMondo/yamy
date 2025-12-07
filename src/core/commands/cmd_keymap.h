#ifndef _CMD_KEYMAP_H
#define _CMD_KEYMAP_H

#include "command_base.h"

class Command_Keymap : public Command<Command_Keymap, const Keymap*>
{
public:
    static constexpr const _TCHAR *Name = _T("Keymap");

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_KEYMAP_H
