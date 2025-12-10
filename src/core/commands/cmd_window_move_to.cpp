#include "cmd_window_move_to.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../../platform/windows/windowstool.h" // For asyncMoveWindow

void Command_WindowMoveTo::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_x);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_y);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_WindowMoveTo::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    yamy::platform::Rect rc, rcd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->getWindowSystem(), i_param, &hwnd, &twt, &rc, &rcd))
        return;
    asyncMoveWindow(hwnd, m_x, m_y);
}

tostream &Command_WindowMoveTo::outputArgs(tostream &i_ost) const
{
    i_ost << m_x << _T(", ");
    i_ost << m_y << _T(", ");
    i_ost << m_twt;
    return i_ost;
}
