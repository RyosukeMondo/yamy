#include "cmd_mouse_hook.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../window/window_system.h" // For WindowPoint
#include "../platform/hook_interface.h" // For platform hook data interface

void Command_MouseHook::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_hookType);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_hookParam);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_MouseHook::exec(Engine *i_engine, FunctionParam *i_param) const
{
    auto* hookData = yamy::platform::getHookData();

    yamy::platform::Point wp;
    i_engine->getWindowSystem()->getCursorPos(&wp);
    hookData->m_mousePos.x = wp.x;
    hookData->m_mousePos.y = wp.y;

    hookData->m_mouseHookType = static_cast<yamy::platform::MouseHookType>(m_hookType);
    hookData->m_mouseHookParam = m_hookParam;

    switch (m_hookType) {
    case MouseHookType_WindowMove: {
        // For this type, hookData->m_mouseHookParam means
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

        hookData->m_hwndMouseHookTarget =
            (uint32_t)((uintptr_t)i_engine->getWindowSystem()->getToplevelWindow(target, &isMDI));
        break;
    }
    default:
        hookData->m_hwndMouseHookTarget = 0;
        break;
    }
}

std::ostream &Command_MouseHook::outputArgs(std::ostream &i_ost) const
{
    i_ost << m_hookType << ", ";
    i_ost << m_hookParam;
    return i_ost;
}
