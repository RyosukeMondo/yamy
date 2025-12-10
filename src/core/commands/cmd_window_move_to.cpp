#include "cmd_window_move_to.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_WindowMoveTo::load(SettingLoader *i_sl)
{
    const char* tName = Name;

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

    int w = rc.width();
    int h = rc.height();
    yamy::platform::Rect newRect(m_x, m_y, m_x + w, m_y + h);
    i_engine->getWindowSystem()->moveWindow(hwnd, newRect);
}

tostream &Command_WindowMoveTo::outputArgs(tostream &i_ost) const
{
    i_ost << m_x << ", ";
    i_ost << m_y << ", ";
    i_ost << m_twt;
    return i_ost;
}
