#include "cmd_window_minimize.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowMinimize::Command_WindowMinimize()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowMinimize::load(SettingLoader *i_sl)
{
    std::string sName = getName();
    const char* cName = sName.c_str();

    if (!i_sl->getOpenParen(false, cName))
      return;
    if (i_sl->getCloseParen(false, cName))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, cName); // throw ...
}

void Command_WindowMinimize::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->getWindowSystem(), i_param, &hwnd, &twt))
        return;

    bool isIconic = (i_engine->getWindowSystem()->getShowCommand(hwnd) == yamy::platform::WindowShowCmd::Minimized);
    i_engine->getWindowSystem()->postMessage(hwnd, WM_SYSCOMMAND,
                isIconic ? SC_RESTORE : SC_MINIMIZE, 0);
}

tostream &Command_WindowMinimize::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
