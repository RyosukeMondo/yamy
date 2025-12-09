#include "cmd_mayu_dialog.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_MayuDialog::load(SettingLoader *i_sl)
{
    i_sl->getOpenParen(true, Name); // throw ...
    i_sl->load_ARGUMENT(&m_dialog);
    i_sl->getComma(false, Name); // throw ...
    i_sl->load_ARGUMENT(&m_showCommand);
    i_sl->getCloseParen(true, Name); // throw ...
}

void Command_MayuDialog::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;
    i_engine->m_windowSystem->postMessage((WindowSystem::WindowHandle)i_engine->getAssociatedWndow(), WM_APP_engineNotify, EngineNotify_showDlg,
                static_cast<LPARAM>(m_dialog) |
                static_cast<LPARAM>(m_showCommand));
}

tostream &Command_MayuDialog::outputArgs(tostream &i_ost) const
{
    i_ost << m_dialog << _T(", ");
    i_ost << m_showCommand;
    return i_ost;
}
