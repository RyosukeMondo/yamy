#include "cmd_recenter.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

#ifndef WM_MAYU_MESSAGE_NAME
#define WM_MAYU_MESSAGE_NAME "YAMY_MAYU_MESSAGE"
#endif
#ifndef MayuMessage_funcRecenter
#define MayuMessage_funcRecenter 1
#endif

void Command_Recenter::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (i_engine->m_hwndFocus) {
        UINT WM_MAYU_MESSAGE = i_engine->getWindowSystem()->registerWindowMessage(
                                   to_UTF_8(addSessionId(WM_MAYU_MESSAGE_NAME)));
        i_engine->getWindowSystem()->postMessage(i_engine->m_hwndFocus, WM_MAYU_MESSAGE, MayuMessage_funcRecenter, 0);
    }
}
