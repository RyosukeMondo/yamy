#include "cmd_log_clear.h"
#include "../engine/engine.h"

void Command_LogClear::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;
    i_engine->getWindowSystem()->postMessage(i_engine->getAssociatedWndow(), WM_APP_engineNotify,
                EngineNotify_clearLog, 0);
}
