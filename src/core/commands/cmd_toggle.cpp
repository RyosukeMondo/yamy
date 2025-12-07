#include "cmd_toggle.h"
#include "../engine/engine.h"
#include "../functions/function.h"
#include <iostream>

void Command_Toggle::exec(Engine *i_engine, FunctionParam *i_param) const
{
    ModifierLockType i_lock = getArg<0>();
    ToggleType i_toggle = getArg<1>();

    if (i_param->m_isPressed)			// ignore PRESS
        return;

    Modifier::Type mt = static_cast<Modifier::Type>(i_lock);
    switch (i_toggle) {
    case ToggleType_toggle:
        i_engine->m_currentLock.press(mt, !i_engine->m_currentLock.isPressed(mt));
        break;
    case ToggleType_off:
        i_engine->m_currentLock.press(mt, false);
        break;
    case ToggleType_on:
        i_engine->m_currentLock.press(mt, true);
        break;
    }
}
