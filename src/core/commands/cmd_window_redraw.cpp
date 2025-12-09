#include "cmd_window_redraw.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_WindowRedraw::exec(Engine *i_engine, FunctionParam *i_param) const
{
    HWND hwnd;
    if (!Engine::getSuitableWindow(i_param, &hwnd))
        return;
    i_engine->m_windowSystem->redrawWindow((WindowSystem::WindowHandle)hwnd);
}
