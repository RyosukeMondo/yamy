#include "cmd_mouse_wheel.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_MouseWheel::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_delta);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_MouseWheel::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    if (i_engine->m_inputInjector) {
        KEYBOARD_INPUT_DATA kid;
        kid.UnitId = 0;
        kid.MakeCode = 10; // Generic Wheel
        kid.Flags = KEYBOARD_INPUT_DATA::E1; // Mouse event
        kid.Reserved = 0;
        kid.ExtraInformation = static_cast<unsigned long>(m_delta);

        yamy::platform::InjectionContext ctx;
        ctx.isDragging = false;

        i_engine->m_inputInjector->inject(&kid, ctx);
    }
}

tostream &Command_MouseWheel::outputArgs(tostream &i_ost) const
{
    i_ost << m_delta;
    return i_ost;
}
