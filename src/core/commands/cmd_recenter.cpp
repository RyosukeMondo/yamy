#include "cmd_recenter.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_Recenter::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (i_engine->m_hwndFocus) {
        UINT WM_MAYU_MESSAGE = i_engine->m_windowSystem->registerWindowMessage(
                                   addSessionId(WM_MAYU_MESSAGE_NAME).c_str());
        i_engine->m_windowSystem->postMessage((WindowSystem::WindowHandle)i_engine->m_hwndFocus, WM_MAYU_MESSAGE, MayuMessage_funcRecenter, 0);
    }
}
