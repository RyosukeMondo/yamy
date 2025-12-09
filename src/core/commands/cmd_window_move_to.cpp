#include "cmd_window_move_to.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../../platform/windows/windowstool.h" // For asyncMoveWindow, rcWidth, rcHeight

Command_WindowMoveTo::Command_WindowMoveTo()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowMoveTo::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_gravityType);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_dx);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_dy);
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

    int x = rc.left + m_dx;
    int y = rc.top + m_dy;

    if (m_gravityType & GravityType_N)
        y = m_dy + rcd.top;
    if (m_gravityType & GravityType_E)
        x = m_dx + rcd.right - rcWidth(reinterpret_cast<const RECT*>(&rc));
    if (m_gravityType & GravityType_W)
        x = m_dx + rcd.left;
    if (m_gravityType & GravityType_S)
        y = m_dy + rcd.bottom - rcHeight(reinterpret_cast<const RECT*>(&rc));
    asyncMoveWindow(static_cast<HWND>(hwnd), x, y);
}

tostream &Command_WindowMoveTo::outputArgs(tostream &i_ost) const
{
    i_ost << m_gravityType << _T(", ");
    i_ost << m_dx << _T(", ");
    i_ost << m_dy << _T(", ");
    i_ost << m_twt;
    return i_ost;
}
