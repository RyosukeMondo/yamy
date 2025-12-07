#ifndef _CMD_VARIABLE_H
#define _CMD_VARIABLE_H

#include "command_base.h"

class Command_Variable : public Command<Command_Variable, int, int>
{
public:
    static constexpr const _TCHAR *Name = _T("Variable");

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_VARIABLE_H
