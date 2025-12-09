#include "cmd_investigate_command.h"
#include "../engine/engine.h"
#include "../functions/hook.h" // For g_hookData

void Command_InvestigateCommand::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;
    Acquire a(&i_engine->m_log, 0);
    g_hookData->m_doesNotifyCommand = !g_hookData->m_doesNotifyCommand;
    if (g_hookData->m_doesNotifyCommand)
        i_engine->m_log << _T(" begin") << std::endl;
    else
        i_engine->m_log << _T(" end") << std::endl;
}
