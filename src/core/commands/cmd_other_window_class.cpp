#include "cmd_other_window_class.h"
#include "../engine/engine.h"
#include "../functions/function.h"
#include "../input/keymap.h"
#include <iostream>
#include "cmd_default.h"
#include "../utils/stringtool.h"

void Command_OtherWindowClass::exec(Engine *i_engine, FunctionParam *i_param) const
{
    Engine::Current c(i_param->m_c);
    ++ c.m_i;
    if (c.m_i == i_engine->m_currentFocusOfThread->m_keymaps.end()) {
        Command_Default defaultCmd;
        defaultCmd.exec(i_engine, i_param);
        return;
    }

    c.m_keymap = *c.m_i;
    {
        Acquire a(&i_engine->m_log, 1);
        i_engine->m_log << "(" << to_tstring(c.m_keymap->getName()) << ")" << std::endl;
    }
    i_param->m_doesNeedEndl = false;
    i_engine->generateKeyboardEvents(c);
}
