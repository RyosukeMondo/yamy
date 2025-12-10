#include "cmd_keymap.h"
#include "../engine/engine.h"
#include "../functions/function.h"
#include "../input/keymap.h"
#include <iostream>

void Command_Keymap::exec(Engine *i_engine, FunctionParam *i_param) const
{
    const Keymap* i_keymap = getArg<0>();

    Engine::Current c(i_param->m_c);
    c.m_keymap = i_keymap;
    {
        Acquire a(&i_engine->m_log, 1);
        i_engine->m_log << "(" << c.m_keymap->getName() << ")" << std::endl;
        i_param->m_doesNeedEndl = false;
    }
    i_engine->generateKeyboardEvents(c);
}
