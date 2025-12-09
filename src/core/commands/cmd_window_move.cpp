#include "cmd_window_move.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../../platform/windows/windowstool.h" // For asyncMoveWindow

Command_WindowMove::Command_WindowMove()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowMove::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_dx);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_dy);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_WindowMove::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    yamy::platform::Rect rc, rcd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->getWindowSystem(), i_param, &hwnd, &twt, &rc, &rcd))
        return;
    asyncMoveWindow(static_cast<HWND>(hwnd), rc.left + m_dx, rc.top + m_dy);
}

tostream &Command_WindowMove::outputArgs(tostream &i_ost) const
{
    i_ost << m_dx << _T(", ");
    i_ost << m_dy << _T(", ");
    i_ost << m_twt;
    return i_ost;
}
