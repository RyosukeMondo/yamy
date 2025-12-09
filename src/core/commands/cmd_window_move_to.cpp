#include "cmd_window_move_to.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowMoveTo::Command_WindowMoveTo()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowMoveTo::load(SettingLoader *i_sl)
{
    i_sl->getOpenParen(true, Name); // throw ...
    i_sl->load_ARGUMENT(&m_gravityType);
    i_sl->getComma(false, Name); // throw ...
    i_sl->load_ARGUMENT(&m_dx);
    i_sl->getComma(false, Name); // throw ...
    i_sl->load_ARGUMENT(&m_dy);
    if (i_sl->getCloseParen(false, Name))
      return;
    i_sl->getComma(false, Name); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, Name); // throw ...
}

void Command_WindowMoveTo::exec(Engine *i_engine, FunctionParam *i_param) const
{
    HWND hwnd;
    RECT rc, rcd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->m_windowSystem, i_param, &hwnd, &twt, &rc, &rcd))
        return;

    int x = rc.left + m_dx;
    int y = rc.top + m_dy;

    if (m_gravityType & GravityType_N)
        y = m_dy + rcd.top;
    if (m_gravityType & GravityType_E)
        x = m_dx + rcd.right - rcWidth(&rc);
    if (m_gravityType & GravityType_W)
        x = m_dx + rcd.left;
    if (m_gravityType & GravityType_S)
        y = m_dy + rcd.bottom - rcHeight(&rc);
    i_engine->asyncMoveWindow(hwnd, x, y);
}

tostream &Command_WindowMoveTo::outputArgs(tostream &i_ost) const
{
    i_ost << m_gravityType << _T(", ");
    i_ost << m_dx << _T(", ");
    i_ost << m_dy << _T(", ");
    i_ost << m_twt;
    return i_ost;
}
