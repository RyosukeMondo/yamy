#include "cmd_window_move_visibly.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../../platform/windows/windowstool.h" // For asyncMoveWindow, rcWidth, rcHeight

Command_WindowMoveVisibly::Command_WindowMoveVisibly()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowMoveVisibly::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    if (!i_sl->getOpenParen(false, tName))
      return;
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_WindowMoveVisibly::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    yamy::platform::Rect rc, rcd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->getWindowSystem(), i_param, &hwnd, &twt, &rc, &rcd))
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
    asyncMoveWindow(hwnd, x, y);
}

tostream &Command_WindowMoveVisibly::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
