#include "cmd_window_move.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowMove::Command_WindowMove()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowMove::load(SettingLoader *i_sl)
{
    const char* tName = Name;

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

    yamy::platform::Rect newRect(rc.left + m_dx, rc.top + m_dy,
                                  rc.right + m_dx, rc.bottom + m_dy);
    i_engine->getWindowSystem()->moveWindow(hwnd, newRect);
}

tostream &Command_WindowMove::outputArgs(tostream &i_ost) const
{
    i_ost << m_dx << ", ";
    i_ost << m_dy << ", ";
    i_ost << m_twt;
    return i_ost;
}
