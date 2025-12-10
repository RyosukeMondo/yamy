#include "cmd_window_monitor_to.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
// #include "../../platform/windows/windowstool.h" // Removed Win32 dependency
#include <vector>

using namespace yamy::platform;

Command_WindowMonitorTo::Command_WindowMonitorTo()
{
    m_adjustPos = BooleanType_true;
    m_adjustSize = BooleanType_false;
}

void Command_WindowMonitorTo::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_fromType);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_monitor);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_adjustPos);
    if (i_sl->getCloseParen(false, tName))
      return;
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_adjustSize);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_WindowMonitorTo::exec(Engine *i_engine, FunctionParam *i_param) const
{
    WindowHandle hwnd;
    IWindowSystem* ws = i_engine->getWindowSystem();

    if (! Engine::getSuitableWindow(ws, i_param, &hwnd))
        return;

    int currentIdx = ws->getMonitorIndex(hwnd);
    int monitorCount = ws->getMonitorCount();

    if (monitorCount < 1 || currentIdx < 0)
        return;

    int targetIdx = 0;
    switch (m_fromType) {
    case WindowMonitorFromType_primary:
        // Assume primary is index 0
        targetIdx = (0 + m_monitor) % monitorCount;
        break;

    case WindowMonitorFromType_current:
        targetIdx = (currentIdx + m_monitor) % monitorCount;
        break;
    }
    if (currentIdx == targetIdx)
        return;

    Rect rcCur, rcTarget, rcWin;
    // Using WorkArea as per original code (mi.rcWork)
    if (!ws->getMonitorWorkArea(currentIdx, &rcCur) ||
        !ws->getMonitorWorkArea(targetIdx, &rcTarget))
        return;

    ws->getWindowRect(hwnd, &rcWin);

    int x = rcTarget.left + (rcWin.left - rcCur.left);
    int y = rcTarget.top + (rcWin.top - rcCur.top);
    int w = rcWin.width();
    int h = rcWin.height();

    if (m_adjustPos) {
        if (x + w > rcTarget.right)
            x = rcTarget.right - w;
        if (x < rcTarget.left)
            x = rcTarget.left;
        if (w > rcTarget.width()) {
            x = rcTarget.left;
            w = rcTarget.width();
        }

        if (y + h > rcTarget.bottom)
            y = rcTarget.bottom - h;
        if (y < rcTarget.top)
            y = rcTarget.top;
        if (h > rcTarget.height()) {
            y = rcTarget.top;
            h = rcTarget.height();
        }
    }

    if (m_adjustPos && m_adjustSize) {
        if (ws->getShowCommand(hwnd) == yamy::platform::WindowShowCmd::Maximized)
            ws->showWindow(hwnd, 9); // SW_RESTORE

        ws->moveWindow(hwnd, Rect(x, y, x+w, y+h));
    } else {
        ws->moveWindow(hwnd, Rect(x, y, x+w, y+h));
    }
}

tostream &Command_WindowMonitorTo::outputArgs(tostream &i_ost) const
{
    i_ost << m_fromType << ", ";
    i_ost << m_monitor << ", ";
    i_ost << m_adjustPos << ", ";
    i_ost << m_adjustSize;
    return i_ost;
}
