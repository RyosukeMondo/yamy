#include "cmd_describe_bindings.h"
#include "../engine/engine.h"

void Command_DescribeBindings::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;
    {
        Acquire a(&i_engine->m_log, 1);
        i_engine->m_log << std::endl;
    }
    i_engine->describeBindings();
}
