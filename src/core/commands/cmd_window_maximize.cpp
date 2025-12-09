#include "cmd_window_maximize.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowMaximize::Command_WindowMaximize()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowMaximize::load(SettingLoader *i_sl)
{
    if (!i_sl->getOpenParen(false, Name))
      return;
    if (i_sl->getCloseParen(false, Name))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, Name); // throw ...
}

void Command_WindowMaximize::exec(Engine *i_engine, FunctionParam *i_param) const
{
    HWND hwnd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->m_windowSystem, i_param, &hwnd, &twt))
        return;

    bool isZoomed = (i_engine->m_windowSystem->getShowCommand((WindowSystem::WindowHandle)hwnd) == WindowShowCmd::Maximized);
    i_engine->m_windowSystem->postMessage((WindowSystem::WindowHandle)hwnd, WM_SYSCOMMAND,
                isZoomed ? SC_RESTORE : SC_MAXIMIZE, 0);
}

tostream &Command_WindowMaximize::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
