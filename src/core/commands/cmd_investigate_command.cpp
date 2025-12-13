#include "cmd_investigate_command.h"
#include "../engine/engine.h"
#include "../platform/hook_interface.h" // For platform hook data interface

void Command_InvestigateCommand::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    auto* hookData = yamy::platform::getHookData();
    Acquire a(&i_engine->m_log, 0);
    hookData->m_doesNotifyCommand = !hookData->m_doesNotifyCommand;
    if (hookData->m_doesNotifyCommand)
        i_engine->m_log << " begin" << std::endl;
    else
        i_engine->m_log << " end" << std::endl;
}
