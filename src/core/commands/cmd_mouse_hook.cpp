#include "cmd_mouse_hook.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../window/window_system.h" // For WindowPoint
#include "../../platform/windows/hook.h" // For g_hookData
#include "../../platform/windows/windowstool.h" // For getToplevelWindow

void Command_MouseHook::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_hookType);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_hookParam);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_MouseHook::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::Point wp;
    i_engine->getWindowSystem()->getCursorPos(&wp);
    g_hookData->m_mousePos.x = wp.x;
    g_hookData->m_mousePos.y = wp.y;

    g_hookData->m_mouseHookType = m_hookType;
    g_hookData->m_mouseHookParam = m_hookParam;

    switch (m_hookType) {
    case MouseHookType_WindowMove: {
        // For this type, g_hookData->m_mouseHookParam means
        // target window type to move.
        yamy::platform::WindowHandle target;
        bool isMDI;

        // i_hooParam < 0 means target window to move is MDI.
        if (m_hookParam < 0)
            isMDI = true;
        else
            isMDI = false;

        // abs(i_hookParam) == 2: target is window under mouse cursor
        // otherwise: target is current focus window
        if (m_hookParam == 2 || m_hookParam == -2)
            target = i_engine->getWindowSystem()->windowFromPoint(wp);
        else
            target = i_param->m_hwnd;

        g_hookData->m_hwndMouseHookTarget =
            (uint32_t)((uintptr_t)getToplevelWindow(target, &isMDI));
        break;
    }
    default:
        g_hookData->m_hwndMouseHookTarget = 0;
        break;
    }
}

tostream &Command_MouseHook::outputArgs(tostream &i_ost) const
{
    i_ost << m_hookType << _T(", ");
    i_ost << m_hookParam;
    return i_ost;
}
