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
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_windowClassName);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_logicalOp);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_windowTitleName);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_SetForegroundWindow::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    yamy::platform::WindowHandle targetHwnd = nullptr;

    i_engine->getWindowSystem()->enumerateWindows([&](WindowSystem::WindowHandle window) -> bool {
        tstring className = i_engine->getWindowSystem()->getClassName(window);
        tsmatch what;
        if (!std::regex_search(className, what, m_windowClassName)) {
            if (m_logicalOp == LogicalOperatorType_and)
                return true; // continue
        }

        if (m_logicalOp == LogicalOperatorType_and) {
            tstring titleName = i_engine->getWindowSystem()->getTitleName(window);
            if (!std::regex_search(titleName, what, m_windowTitleName))
                return true; // continue
        }

        targetHwnd = (yamy::platform::WindowHandle)window;
        return false; // stop
    });

    if (targetHwnd)
        i_engine->getWindowSystem()->postMessage(i_engine->m_hwndAssocWindow,
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
