//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_input.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>


unsigned int Engine::injectInput(const KEYBOARD_INPUT_DATA *i_kid, const void *i_kidRaw)
{
    if (i_kid->ExtraInformation == 0x59414D59) {
        // Mouse event
        bool down = !(i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK);
        using namespace yamy::platform;

        switch (i_kid->MakeCode) {
            case 1: m_inputInjector->mouseButton(MouseButton::Left, down); break;
            case 2: m_inputInjector->mouseButton(MouseButton::Right, down); break;
            case 3: m_inputInjector->mouseButton(MouseButton::Middle, down); break;
            case 4: if (!down) m_inputInjector->mouseWheel(120); break; // WHEEL_DELTA
            case 5: if (!down) m_inputInjector->mouseWheel(-120); break;
            case 6: m_inputInjector->mouseButton(MouseButton::X1, down); break;
            case 7: m_inputInjector->mouseButton(MouseButton::X2, down); break;
            case 8: if (!down) m_inputInjector->mouseWheel(120); break; // HWheel not supported in interface yet, treating as V
            case 9: if (!down) m_inputInjector->mouseWheel(-120); break;
            case 10: if (!down) m_inputInjector->mouseWheel(static_cast<int32_t>(i_kid->ExtraInformation)); break; // Invalid cast? ExtraInfo is magic
            default: break;
        }
    } else {
        // Keyboard event
        yamy::platform::InjectionContext ctx;
        ctx.isDragging = false;
        ctx.dragStartPos = yamy::platform::Point(0, 0);

        m_inputInjector->inject(i_kid, ctx, i_kidRaw);
    }
    return 1;
}


// pop all pressed key on win32
void Engine::keyboardResetOnWin32()
{
    for (Keyboard::KeyIterator
            i = m_setting->m_keyboard.getKeyIterator();  *i; ++ i) {
        if ((*i)->m_isPressedOnWin32)
            generateKeyEvent((*i), false, true);
    }
}
