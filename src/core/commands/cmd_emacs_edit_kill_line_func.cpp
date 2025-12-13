#include "cmd_emacs_edit_kill_line_func.h"
#include "../engine/engine.h"

void Command_EmacsEditKillLineFunc::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;
    i_engine->m_emacsEditKillLine.func(i_engine->getWindowSystem());
    i_engine->m_emacsEditKillLine.m_doForceReset = false;
}
