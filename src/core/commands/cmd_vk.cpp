#include "cmd_vk.h"
#include "../engine/engine.h"
#include "../functions/function.h"
#include "../../platform/windows/driver.h" // For KEYBOARD_INPUT_DATA
#include "../input/input_injector.h" // For InjectionContext

void Command_VK::exec(Engine *i_engine, FunctionParam *i_param) const
{
    // Logic from Engine::funcVK
    VKey i_vkey = std::get<0>(m_args);
    long key = static_cast<long>(i_vkey);
    BYTE vkey = static_cast<BYTE>(i_vkey);
    bool isExtended = !!(key & VKey_extended);
    bool isUp       = !i_param->m_isPressed && !!(key & VKey_released);
    bool isDown     = i_param->m_isPressed && !!(key & VKey_pressed);

    if (!isUp && !isDown)
        return;

    KEYBOARD_INPUT_DATA kid;
    kid.UnitId = 0;
    kid.ExtraInformation = 0;
    kid.Reserved = 0;

    if (vkey == VK_LBUTTON || vkey == VK_RBUTTON || vkey == VK_MBUTTON ||
        vkey == VK_XBUTTON1 || vkey == VK_XBUTTON2) {
        // Mouse event
        kid.Flags = KEYBOARD_INPUT_DATA::E1;
        if (isUp)
            kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;

        if (vkey == VK_LBUTTON)
            kid.MakeCode = 1;
        else if (vkey == VK_RBUTTON)
            kid.MakeCode = 2;
        else if (vkey == VK_MBUTTON)
            kid.MakeCode = 3;
        else if (vkey == VK_XBUTTON1)
            kid.MakeCode = 6;
        else if (vkey == VK_XBUTTON2)
            kid.MakeCode = 7;
    } else {
        // Keyboard event
        kid.Flags = 0;
        if (isExtended)
            kid.Flags |= KEYBOARD_INPUT_DATA::E0;
        if (isUp)
            kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;

        // Use WindowSystem to map virtual key to scan code
        kid.MakeCode = static_cast<unsigned short>(i_engine->m_windowSystem->mapVirtualKey(vkey));
    }

    InjectionContext ctx;
    ctx.isDragging = false; // funcVK doesn't seem to support dragging context explicitly?
    
    if (i_engine->m_inputInjector) {
        i_engine->m_inputInjector->inject(&kid, ctx);
    }
}
