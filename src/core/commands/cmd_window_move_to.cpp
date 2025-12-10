#include "cmd_window_move_to.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowMoveTo::Command_WindowMoveTo()
    : m_gravityType(GravityType_NW)
    , m_dx(0)
    , m_dy(0)
    , m_twt(TargetWindowType_overlapped)
{
}

void Command_WindowMoveTo::load(SettingLoader *i_sl)
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

void Command_WindowMoveTo::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    yamy::platform::Rect rc, rcd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->getWindowSystem(), i_param, &hwnd, &twt, &rc, &rcd))
        return;

    int w = rc.width();
    int h = rc.height();
    yamy::platform::Rect newRect(m_dx, m_dy, m_dx + w, m_dy + h);
    i_engine->getWindowSystem()->moveWindow(hwnd, newRect);
}

tostream &Command_WindowMoveTo::outputArgs(tostream &i_ost) const
{
    i_ost << m_dx << ", ";
    i_ost << m_dy << ", ";
    i_ost << m_twt;
    return i_ost;
}
