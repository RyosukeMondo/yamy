#include "cmd_mouse_hook.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../functions/hook.h" // For g_hookData

void Command_MouseHook::load(SettingLoader *i_sl)
{
    i_sl->getOpenParen(true, Name); // throw ...
    i_sl->load_ARGUMENT(&m_hookType);
    i_sl->getComma(false, Name); // throw ...
    i_sl->load_ARGUMENT(&m_hookParam);
    i_sl->getCloseParen(true, Name); // throw ...
}

void Command_MouseHook::exec(Engine *i_engine, FunctionParam *i_param) const
{
    WindowPoint wp;
    i_engine->m_windowSystem->getCursorPos(&wp);
    g_hookData->m_mousePos.x = wp.x;
    g_hookData->m_mousePos.y = wp.y;

    g_hookData->m_mouseHookType = m_hookType;
    g_hookData->m_mouseHookParam = m_hookParam;

    switch (m_hookType) {
    case MouseHookType_WindowMove: {
        // For this type, g_hookData->m_mouseHookParam means
        // target window type to move.
        HWND target;
        bool isMDI;

        // i_hooParam < 0 means target window to move is MDI.
        if (m_hookParam < 0)
            isMDI = true;
        else
            isMDI = false;

        // abs(i_hookParam) == 2: target is window under mouse cursor
        // otherwise: target is current focus window
        if (m_hookParam == 2 || m_hookParam == -2)
            target = (HWND)i_engine->m_windowSystem->windowFromPoint(wp);
        else
            target = i_param->m_hwnd;

        g_hookData->m_hwndMouseHookTarget =
            (DWORD)((ULONG_PTR)getToplevelWindow(target, &isMDI));
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
