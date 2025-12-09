#include "cmd_keymap_parent.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For FunctionParam
#include "../input/keymap.h"
#include <iostream>
#include "cmd_default.h"

void Command_KeymapParent::exec(Engine *i_engine, FunctionParam *i_param) const
{
    Engine::Current c(i_param->m_c);
    c.m_keymap = c.m_keymap->getParentKeymap();
    if (!c.m_keymap) {
        Command_Default defaultCmd;
        defaultCmd.exec(i_engine, i_param);
        return;
    }

    {
        Acquire a(&i_engine->m_log, 1);
        i_engine->m_log << _T("(") << c.m_keymap->getName() << _T(")") << std::endl;
    }
    i_param->m_doesNeedEndl = false;
    i_engine->generateKeyboardEvents(c);
}
