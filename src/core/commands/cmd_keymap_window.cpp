#include "cmd_keymap_window.h"
#include "../engine/engine.h"
#include "../functions/function.h"
#include "../input/keymap.h"
#include <iostream>

void Command_KeymapWindow::exec(Engine *i_engine, FunctionParam *i_param) const
{
    Engine::Current c(i_param->m_c);
    c.m_keymap = i_engine->m_currentFocusOfThread->m_keymaps.front();
    c.m_i = i_engine->m_currentFocusOfThread->m_keymaps.begin();
    i_engine->generateKeyboardEvents(c);
}
