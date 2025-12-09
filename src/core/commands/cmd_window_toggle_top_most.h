#pragma once
#ifndef _CMD_WINDOW_TOGGLE_TOP_MOST_H
#define _CMD_WINDOW_TOGGLE_TOP_MOST_H

#include "command_base.h"

class Command_WindowToggleTopMost : public Command<Command_WindowToggleTopMost>
{
public:
    static constexpr const _TCHAR *Name = _T("WindowToggleTopMost");

    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_WINDOW_TOGGLE_TOP_MOST_H
