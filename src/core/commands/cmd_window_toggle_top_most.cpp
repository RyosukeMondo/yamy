#include "cmd_window_toggle_top_most.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_WindowToggleTopMost::exec(Engine *i_engine, FunctionParam *i_param) const
{
    HWND hwnd;
    if (!Engine::getSuitableWindow(i_param, &hwnd))
        return;

    ZOrder order = i_engine->m_windowSystem->isWindowTopMost((WindowSystem::WindowHandle)hwnd)
        ? ZOrder::NoTopMost
        : ZOrder::TopMost;

    i_engine->m_windowSystem->setWindowZOrder((WindowSystem::WindowHandle)hwnd, order);
}
