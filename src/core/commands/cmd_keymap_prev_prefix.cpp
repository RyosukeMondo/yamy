#include "cmd_keymap_prev_prefix.h"
#include "../engine/engine.h"

void Command_KeymapPrevPrefix::exec(Engine *i_engine, FunctionParam *i_param) const
{
    Engine::Current c(i_param->m_c);
    int i_previous = getArg<0>();
    if (0 < i_previous && 0 <= i_engine->m_keymapPrefixHistory.size() - i_previous) {
        int n = i_previous - 1;
        Engine::KeymapPtrList::reverse_iterator i = i_engine->m_keymapPrefixHistory.rbegin();
        while (0 < n && i != i_engine->m_keymapPrefixHistory.rend())
            --n, ++i;
        c.m_keymap = *i;
        i_engine->generateKeyboardEvents(c);
    }
}
