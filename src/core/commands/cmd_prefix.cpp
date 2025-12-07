#include "cmd_prefix.h"
#include "../engine/engine.h"
#include "../functions/function.h"
#include "../input/keymap.h"
#include <iostream>

void Command_Prefix::exec(Engine *i_engine, FunctionParam *i_param) const
{
    const Keymap* i_keymap = getArg<0>();
    BooleanType i_doesIgnoreModifiers = getArg<1>();

    if (!i_param->m_isPressed)
        return;

    i_engine->setCurrentKeymap(i_keymap, true);

    // generate prefixed event
    i_engine->generateEvents(i_param->m_c, i_engine->m_currentKeymap, &Event::prefixed);

    i_engine->m_isPrefix = true;
    i_engine->m_doesEditNextModifier = false;
    i_engine->m_doesIgnoreModifierForPrefix = !!i_doesIgnoreModifiers;

    {
        Acquire a(&i_engine->m_log, 1);
        i_engine->m_log << _T("(") << i_keymap->getName() << _T(", ")
                        << (i_doesIgnoreModifiers ? _T("true") : _T("false")) << _T(")");
    }
}
