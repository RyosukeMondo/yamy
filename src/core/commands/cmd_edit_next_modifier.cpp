#include "cmd_edit_next_modifier.h"
#include "../engine/engine.h"
#include "../functions/function.h"
#include <iostream>

void Command_EditNextModifier::exec(Engine *i_engine, FunctionParam *i_param) const
{
    const Modifier& i_modifier = getArg<0>();

    if (!i_param->m_isPressed)
        return;

    i_engine->m_isPrefix = true;
    i_engine->m_doesEditNextModifier = true;
    i_engine->m_modifierForNextKey.add(i_modifier);
}
