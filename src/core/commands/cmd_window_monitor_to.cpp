#include "cmd_window_monitor_to.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../../platform/windows/windowstool.h" // For asyncMoveWindow, rcWidth, rcHeight
#include <vector>

// Helper structs for WindowMonitorTo
struct EnumDisplayMonitorsForWindowMonitorToParam {
    std::vector<HMONITOR> m_monitors;
    std::vector<MONITORINFO> m_monitorinfos;
    int m_primaryMonitorIdx;
    int m_currentMonitorIdx;

    HMONITOR m_hmon;

public:
    EnumDisplayMonitorsForWindowMonitorToParam(HMONITOR i_hmon)
            : m_primaryMonitorIdx(-1), m_currentMonitorIdx(-1), m_hmon(i_hmon) {
    }
};

static BOOL CALLBACK enumDisplayMonitorsForWindowMonitorTo(
    HMONITOR i_hmon, HDC i_hdc, LPRECT i_rcMonitor, LPARAM i_data)
{
    EnumDisplayMonitorsForWindowMonitorToParam &ep =
        *reinterpret_cast<EnumDisplayMonitorsForWindowMonitorToParam *>(i_data);

    ep.m_monitors.push_back(i_hmon);

    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(i_hmon, &mi);
    ep.m_monitorinfos.push_back(mi);

    if (mi.dwFlags & MONITORINFOF_PRIMARY)
        ep.m_primaryMonitorIdx = (int)(ep.m_monitors.size() - 1);
    if (i_hmon == ep.m_hmon)
        ep.m_currentMonitorIdx = (int)(ep.m_monitors.size() - 1);

    return TRUE;
}

Command_WindowMonitorTo::Command_WindowMonitorTo()
{
    m_adjustPos = BooleanType_true;
    m_adjustSize = BooleanType_false;
}

void Command_WindowMonitorTo::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

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
    HWND hwnd;
    if (! Engine::getSuitableWindow(i_param, &hwnd))
        return;

    HMONITOR hmonCur;
    hmonCur = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    EnumDisplayMonitorsForWindowMonitorToParam ep(hmonCur);
    EnumDisplayMonitors(nullptr, nullptr, enumDisplayMonitorsForWindowMonitorTo,
                        reinterpret_cast<LPARAM>(&ep));
    if (ep.m_monitors.size() < 1 ||
            ep.m_primaryMonitorIdx < 0 || ep.m_currentMonitorIdx < 0)
        return;

    int targetIdx = 0;
    switch (m_fromType) {
    case WindowMonitorFromType_primary:
        targetIdx = (ep.m_primaryMonitorIdx + m_monitor) % (int)ep.m_monitors.size();
        break;

    case WindowMonitorFromType_current:
        targetIdx = (ep.m_currentMonitorIdx + m_monitor) % (int)ep.m_monitors.size();
        break;
    }
    if (ep.m_currentMonitorIdx == targetIdx)
        return;

    RECT rcCur, rcTarget, rcWin;
    rcCur = ep.m_monitorinfos[ep.m_currentMonitorIdx].rcWork;
    rcTarget = ep.m_monitorinfos[targetIdx].rcWork;
    GetWindowRect(hwnd, &rcWin);

    int x = rcTarget.left + (rcWin.left - rcCur.left);
    int y = rcTarget.top + (rcWin.top - rcCur.top);
    int w = rcWidth(&rcWin);
    int h = rcHeight(&rcWin);

    if (m_adjustPos) {
        if (x + w > rcTarget.right)
            x = rcTarget.right - w;
        if (x < rcTarget.left)
            x = rcTarget.left;
        if (w > rcWidth(&rcTarget)) {
            x = rcTarget.left;
            w = rcWidth(&rcTarget);
        }

        if (y + h > rcTarget.bottom)
            y = rcTarget.bottom - h;
        if (y < rcTarget.top)
            y = rcTarget.top;
        if (h > rcHeight(&rcTarget)) {
            y = rcTarget.top;
            h = rcHeight(&rcTarget);
        }
    }

    if (m_adjustPos && m_adjustSize) {
        if (i_engine->m_windowSystem->getShowCommand((WindowSystem::WindowHandle)hwnd) == WindowShowCmd::Maximized)
            i_engine->m_windowSystem->postMessage((WindowSystem::WindowHandle)hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        asyncMoveWindow(hwnd, x, y, w, h);
    } else {
        asyncMoveWindow(hwnd, x, y);
    }
}

tostream &Command_WindowMonitorTo::outputArgs(tostream &i_ost) const
{
    i_ost << m_fromType << _T(", ");
    i_ost << m_monitor << _T(", ");
    i_ost << m_adjustPos << _T(", ");
    i_ost << m_adjustSize;
    return i_ost;
}
