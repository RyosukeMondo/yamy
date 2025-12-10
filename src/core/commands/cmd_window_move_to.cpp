#include "cmd_window_move_to.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../../platform/windows/windowstool.h" // For asyncMoveWindow

Command_WindowMoveTo::Command_WindowMoveTo()
{
    m_gravityType = GravityType_NW;
    m_dx = 0;
    m_dy = 0;
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowMoveTo::load(SettingLoader *i_sl)
{
    std::string sName = getName();
    const char* cName = sName.c_str();

    i_sl->getOpenParen(true, cName); // throw ...
    i_sl->load_ARGUMENT(&m_dx);
    i_sl->getComma(false, cName); // throw ...
    i_sl->load_ARGUMENT(&m_dy);
    if (i_sl->getCloseParen(false, cName))
      return;
    i_sl->getComma(false, cName); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, cName); // throw ...
}

void Command_WindowMoveTo::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    yamy::platform::Rect rc, rcd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->getWindowSystem(), i_param, &hwnd, &twt, &rc, &rcd))
        return;
    asyncMoveWindow(static_cast<HWND>(hwnd), m_dx, m_dy);
}

tostream &Command_WindowMoveTo::outputArgs(tostream &i_ost) const
{
    i_ost << m_dx << _T(", ");
    i_ost << m_dy << _T(", ");
    i_ost << m_twt;
    return i_ost;
}
