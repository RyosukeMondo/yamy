#include "cmd_shell_execute.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_ShellExecute::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_operation);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_file);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_parameters);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_directory);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_showCommand);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_ShellExecute::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;
    i_engine->m_afShellExecute = i_param->m_af;
    i_engine->m_windowSystem->postMessage((WindowSystem::WindowHandle)i_engine->m_hwndAssocWindow,
                WM_APP_engineNotify, EngineNotify_shellExecute, 0);
}

tostream &Command_ShellExecute::output(tostream &i_ost) const
{
    i_ost << _T("&") << to_tstring(getName()) << _T("(");
    i_ost << m_operation << _T(", ");
    i_ost << m_file << _T(", ");
    i_ost << m_parameters << _T(", ");
    i_ost << m_directory << _T(", ");
    i_ost << m_showCommand;
    i_ost << _T(") ");
    return i_ost;
}
