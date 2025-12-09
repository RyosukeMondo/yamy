#include "cmd_window_toggle_top_most.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_WindowToggleTopMost::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    if (!Engine::getSuitableWindow(i_engine->getWindowSystem(), i_param, &hwnd))
        return;

    ZOrder order = i_engine->getWindowSystem()->isWindowTopMost(hwnd)
        ? ZOrder::NoTopMost
        : ZOrder::TopMost;

    i_engine->getWindowSystem()->setWindowZOrder(hwnd, order);
}
