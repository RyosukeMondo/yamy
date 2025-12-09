#pragma once
#ifndef _CMD_DIRECT_SSTP_H
#define _CMD_DIRECT_SSTP_H

#include "command_base.h"
#include <list>

class Command_DirectSSTP : public Command<Command_DirectSSTP>
{
public:
    static constexpr const char *Name = "DirectSSTP";

    Regex m_name;
    StrExprArg m_protocol;
    std::list<std::string> m_headers;

    virtual void load(SettingLoader *i_sl) override;
    virtual void exec(Engine *i_engine, FunctionParam *i_param) const override;
    virtual tostream &outputArgs(tostream &i_ost) const override;
};

#endif // _CMD_DIRECT_SSTP_H
