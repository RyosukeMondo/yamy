#include "cmd_window_hv_maximize.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../../platform/windows/windowstool.h" // For rcWidth, rcHeight

Command_WindowHVMaximize::Command_WindowHVMaximize()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowHVMaximize::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_isHorizontal);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_WindowHVMaximize::exec(Engine *i_engine, FunctionParam *i_param) const
{
    HWND hwnd;
    RECT rc, rcd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->m_windowSystem, i_param, &hwnd, &twt, &rc, &rcd))
        return;

    int x = rc.left;
    int y = rc.top;
    int w = rcWidth(&rc);
    int h = rcHeight(&rc);

    if (m_isHorizontal) {
        x = rcd.left;
        w = rcWidth(&rcd);
    } else {
        y = rcd.top;
        h = rcHeight(&rcd);
    }
    asyncMoveWindow(hwnd, x, y, w, h);
}

tostream &Command_WindowHVMaximize::outputArgs(tostream &i_ost) const
{
    i_ost << m_isHorizontal << _T(", ");
    i_ost << m_twt;
    return i_ost;
}
