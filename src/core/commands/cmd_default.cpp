#include "cmd_default.h"
#include "../engine/engine.h"

void Command_Default::exec(Engine *i_engine, FunctionParam *i_param) const
{
    {
        Acquire a(&i_engine->m_log, 1);
        i_engine->m_log << std::endl;
        i_param->m_doesNeedEndl = false;
    }
    if (i_param->m_isPressed)
        i_engine->generateModifierEvents(i_param->m_c.m_mkey.m_modifier);
    i_engine->generateKeyEvent(i_param->m_c.m_mkey.m_key, i_param->m_isPressed, true);
}
