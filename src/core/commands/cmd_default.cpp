#include "cmd_default.h"
#include "../engine/engine.h"

void Command_Default::exec(Engine *i_engine, FunctionParam *i_param) const
{
    i_engine->funcDefault(i_param);
}
