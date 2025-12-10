#include "cmd_window_hv_maximize.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../../platform/windows/windowstool.h" // For asyncMoveWindow

void Command_WindowHVMaximize::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_WindowHVMaximize::exec(Engine *i_engine, FunctionParam *i_param) const
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
        int monitorIndex = i_engine->getWindowSystem()->getMonitorIndex(hwnd);
        i_engine->getWindowSystem()->getMonitorWorkArea(monitorIndex, &monitorWorkArea);
    }
    asyncMoveWindow(hwnd, monitorWorkArea.left, monitorWorkArea.top,
                    monitorWorkArea.width(), monitorWorkArea.height());
}

tostream &Command_WindowHVMaximize::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
