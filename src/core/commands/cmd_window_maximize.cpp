#include "cmd_window_maximize.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowMaximize::Command_WindowMaximize()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowMaximize::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    if (!i_sl->getOpenParen(false, tName))
      return;
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_WindowMaximize::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->getWindowSystem(), i_param, &hwnd, &twt))
        return;

    bool isZoomed = (i_engine->getWindowSystem()->getShowCommand(hwnd) == yamy::platform::WindowShowCmd::Maximized);
    i_engine->getWindowSystem()->postMessage(hwnd, WM_SYSCOMMAND,
                isZoomed ? SC_RESTORE : SC_MAXIMIZE, 0);
}

tostream &Command_WindowMaximize::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
