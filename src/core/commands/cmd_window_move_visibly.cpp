#include "cmd_window_move_visibly.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowMoveVisibly::Command_WindowMoveVisibly()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowMoveVisibly::load(SettingLoader *i_sl)
{
    if (!i_sl->getOpenParen(false, Name))
      return;
    if (i_sl->getCloseParen(false, Name))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, Name); // throw ...
}

void Command_WindowMoveVisibly::exec(Engine *i_engine, FunctionParam *i_param) const
{
    HWND hwnd;
    RECT rc, rcd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->m_windowSystem, i_param, &hwnd, &twt, &rc, &rcd))
        return;

    int x = rc.left;
    int y = rc.top;
    if (rc.left < rcd.left)
        x = rcd.left;
    else if (rcd.right < rc.right)
        x = rcd.right - rcWidth(&rc);
    if (rc.top < rcd.top)
        y = rcd.top;
    else if (rcd.bottom < rc.bottom)
        y = rcd.bottom - rcHeight(&rc);
    i_engine->asyncMoveWindow(hwnd, x, y);
}

tostream &Command_WindowMoveVisibly::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
