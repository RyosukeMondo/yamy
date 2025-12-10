#include "cmd_window_move_visibly.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../../platform/windows/windowstool.h" // For asyncMoveWindow

Command_WindowMoveVisibly::Command_WindowMoveVisibly()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowMoveVisibly::load(SettingLoader *i_sl)
{
    std::string sName = getName();
    const char* cName = sName.c_str();

    i_sl->getOpenParen(true, cName); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, cName); // throw ...
}

void Command_WindowMoveVisibly::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    yamy::platform::Rect rc, rcd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->getWindowSystem(), i_param, &hwnd, &twt, &rc, &rcd))
        return;

    yamy::platform::Rect monitorWorkArea;

    if (twt == TargetWindowType_mdi) {
        monitorWorkArea = rcd;
    } else {
        // use getMonitorWorkArea interface instead of MonitorFromWindow
        int monitorIndex = i_engine->getWindowSystem()->getMonitorIndex(hwnd);
        i_engine->getWindowSystem()->getMonitorWorkArea(monitorIndex, &monitorWorkArea);
    }

    if (isRectInRect(reinterpret_cast<const RECT*>(&rc), reinterpret_cast<const RECT*>(&monitorWorkArea)))
        return;

    int w = rc.width();
    int h = rc.height();
    int mw = monitorWorkArea.width();
    int mh = monitorWorkArea.height();

    int x, y;
    if (w > mw)
        x = monitorWorkArea.left;
    else if (rc.left < monitorWorkArea.left)
        x = monitorWorkArea.left;
    else if (rc.right > monitorWorkArea.right)
        x = monitorWorkArea.right - w;
    else
        x = rc.left;

    if (h > mh)
        y = monitorWorkArea.top;
    else if (rc.top < monitorWorkArea.top)
        y = monitorWorkArea.top;
    else if (rc.bottom > monitorWorkArea.bottom)
        y = monitorWorkArea.bottom - h;
    else
        y = rc.top;

    asyncMoveWindow(static_cast<HWND>(hwnd), x, y);
}

tostream &Command_WindowMoveVisibly::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
