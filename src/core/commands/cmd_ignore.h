#pragma once
#ifndef _CMD_IGNORE_H
#define _CMD_IGNORE_H

#include "command_base.h"

class Command_Ignore : public Command<Command_Ignore>
{
public:
	static constexpr const _TCHAR *Name = _T("Ignore");

	virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_IGNORE_H
