#include "cmd_wait.h"
#include "../engine/engine.h"
#include "../functions/function.h"
#include "core/platform/thread.h"

void Command_Wait::exec(Engine *i_engine, FunctionParam *i_param) const
{
    // Logic from Engine::funcWait
    int milliSecond = std::get<0>(m_args);
    if (!i_param->m_isPressed)
        return;
    if (milliSecond < 0 || 5000 < milliSecond)    // too long wait
        return;

    // Friend access to Engine private members
    i_engine->m_isSynchronizing = true;
    i_engine->m_cs.release();
    yamy::platform::sleep_ms(milliSecond);
    i_engine->m_cs.acquire();
    i_engine->m_isSynchronizing = false;
}
