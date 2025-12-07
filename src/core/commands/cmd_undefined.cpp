#include "cmd_undefined.h"
#include "../engine/engine.h"
#include "../functions/function.h"

void Command_Undefined::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;
    MessageBeep(MB_OK);
}
