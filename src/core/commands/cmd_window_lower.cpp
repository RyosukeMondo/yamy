#include "cmd_window_lower.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowLower::Command_WindowLower()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowLower::load(SettingLoader *i_sl)
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

void Command_WindowLower::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->getWindowSystem(), i_param, &hwnd, &twt))
        return;
    i_engine->getWindowSystem()->setWindowZOrder(hwnd, yamy::platform::ZOrder::Bottom);
}

tostream &Command_WindowLower::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
