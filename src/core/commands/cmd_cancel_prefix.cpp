#include "cmd_cancel_prefix.h"
#include "../engine/engine.h"

void Command_CancelPrefix::exec(Engine *i_engine, FunctionParam *i_param) const
{
    i_engine->m_isPrefix = false;
}
