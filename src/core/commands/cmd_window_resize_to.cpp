#include "cmd_window_resize_to.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowResizeTo::Command_WindowResizeTo()
    : m_width(0)
    , m_height(0)
    , m_twt(TargetWindowType_overlapped)
{
}

void Command_WindowResizeTo::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_width);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_height);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_WindowResizeTo::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    yamy::platform::Rect rc, rcd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->getWindowSystem(), i_param, &hwnd, &twt, &rc, &rcd))
        return;

    yamy::platform::Rect newRect(rc.left, rc.top, rc.left + m_width, rc.top + m_height);
    i_engine->getWindowSystem()->moveWindow(hwnd, newRect);
}

tostream &Command_WindowResizeTo::outputArgs(tostream &i_ost) const
{
    i_ost << m_width << ", ";
    i_ost << m_height << ", ";
    i_ost << m_twt;
    return i_ost;
}
