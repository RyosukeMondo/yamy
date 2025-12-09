#include "cmd_window_close.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowClose::Command_WindowClose()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowClose::load(SettingLoader *i_sl)
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

void Command_WindowClose::exec(Engine *i_engine, FunctionParam *i_param) const
{
    HWND hwnd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->m_windowSystem, i_param, &hwnd, &twt))
        return;
    i_engine->m_windowSystem->postMessage((WindowSystem::WindowHandle)hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
}

tostream &Command_WindowClose::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
