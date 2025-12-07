#pragma once
#ifndef _CMD_UNDEFINED_H
#define _CMD_UNDEFINED_H

#include "command_base.h"

class Command_Undefined : public Command<Command_Undefined>
{
public:
	static constexpr const _TCHAR *Name = _T("Undefined");

	virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_UNDEFINED_H
