#include "cmd_set_foreground_window.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_SetForegroundWindow::Command_SetForegroundWindow()
{
    m_logicalOp = LogicalOperatorType_and;
    m_windowTitleName = tregex(_T(".*"));
}

void Command_SetForegroundWindow::load(SettingLoader *i_sl)
{
    i_sl->getOpenParen(true, Name); // throw ...
    i_sl->load_ARGUMENT(&m_windowClassName);
    if (i_sl->getCloseParen(false, Name))
      return;
    i_sl->getComma(false, Name); // throw ...
    i_sl->load_ARGUMENT(&m_logicalOp);
    if (i_sl->getCloseParen(false, Name))
      return;
    i_sl->getComma(false, Name); // throw ...
    i_sl->load_ARGUMENT(&m_windowTitleName);
    i_sl->getCloseParen(true, Name); // throw ...
}

void Command_SetForegroundWindow::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    HWND targetHwnd = nullptr;

    i_engine->m_windowSystem->enumerateWindows([&](WindowSystem::WindowHandle window) -> bool {
        tstring className = i_engine->m_windowSystem->getClassName(window);
        tsmatch what;
        if (!std::regex_search(className, what, m_windowClassName)) {
            if (m_logicalOp == LogicalOperatorType_and)
                return true; // continue
        }

        if (m_logicalOp == LogicalOperatorType_and) {
            tstring titleName = i_engine->m_windowSystem->getTitleName(window);
            if (!std::regex_search(titleName, what, m_windowTitleName))
                return true; // continue
        }

        targetHwnd = (HWND)window;
        return false; // stop
    });

    if (targetHwnd)
        i_engine->m_windowSystem->postMessage((WindowSystem::WindowHandle)i_engine->m_hwndAssocWindow,
                    WM_APP_engineNotify, EngineNotify_setForegroundWindow,
                    reinterpret_cast<LPARAM>(targetHwnd));
}

tostream &Command_SetForegroundWindow::outputArgs(tostream &i_ost) const
{
    i_ost << m_windowClassName << _T(", ");
    i_ost << m_logicalOp << _T(", ");
    i_ost << m_windowTitleName;
    return i_ost;
}
