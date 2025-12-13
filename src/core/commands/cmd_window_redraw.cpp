#include "cmd_window_redraw.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_WindowRedraw::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    if (!Engine::getSuitableWindow(i_engine->getWindowSystem(), i_param, &hwnd))
        return;
    i_engine->getWindowSystem()->redrawWindow(hwnd);
}
