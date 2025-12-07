#include "cmd_variable.h"
#include "../engine/engine.h"
#include "../functions/function.h"
#include <iostream>

void Command_Variable::exec(Engine *i_engine, FunctionParam *i_param) const
{
    int i_mag = getArg<0>();
    int i_inc = getArg<1>();

    if (!i_param->m_isPressed)
        return;

    i_engine->m_variable = i_engine->m_variable * i_mag + i_inc;
}
