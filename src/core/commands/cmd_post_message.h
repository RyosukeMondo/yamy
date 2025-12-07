#pragma once
#ifndef _CMD_POST_MESSAGE_H
#define _CMD_POST_MESSAGE_H

#include "command_base.h"
#include "../functions/function.h" // For ToWindowType

class Command_PostMessage : public Command<Command_PostMessage, ToWindowType, UINT, WPARAM, LPARAM>
{
public:
	static constexpr const _TCHAR *Name = _T("PostMessage");

	Command_PostMessage() = default;

	virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
};

#endif // _CMD_POST_MESSAGE_H
