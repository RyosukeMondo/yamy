#include "cmd_window_minimize.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowMinimize::Command_WindowMinimize()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowMinimize::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    if (!i_sl->getOpenParen(false, tName))
      return;
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_WindowMinimize::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->getWindowSystem(), i_param, &hwnd, &twt))
        return;

    bool isIconic = (i_engine->getWindowSystem()->getShowCommand(hwnd) == WindowShowCmd::Minimized);
    i_engine->getWindowSystem()->postMessage(hwnd, WM_SYSCOMMAND,
                isIconic ? SC_RESTORE : SC_MINIMIZE, 0);
}

tostream &Command_WindowMinimize::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
