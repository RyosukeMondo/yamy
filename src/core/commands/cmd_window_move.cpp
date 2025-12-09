#include "cmd_window_move.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowMove::Command_WindowMove()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowMove::load(SettingLoader *i_sl)
{
    i_sl->getOpenParen(true, Name); // throw ...
    i_sl->load_ARGUMENT(&m_dx);
    i_sl->getComma(false, Name); // throw ...
    i_sl->load_ARGUMENT(&m_dy);
    if (i_sl->getCloseParen(false, Name))
      return;
    i_sl->getComma(false, Name); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, Name); // throw ...
}

void Command_WindowMove::exec(Engine *i_engine, FunctionParam *i_param) const
{
    HWND hwnd;
    RECT rc, rcd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->m_windowSystem, i_param, &hwnd, &twt, &rc, &rcd))
        return;
    i_engine->asyncMoveWindow(hwnd, rc.left + m_dx, rc.top + m_dy);
}

tostream &Command_WindowMove::outputArgs(tostream &i_ost) const
{
    i_ost << m_dx << _T(", ");
    i_ost << m_dy << _T(", ");
    i_ost << m_twt;
    return i_ost;
}
