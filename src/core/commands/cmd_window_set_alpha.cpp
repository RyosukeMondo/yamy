#include "cmd_window_set_alpha.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include <algorithm>

void Command_WindowSetAlpha::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_alpha);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_WindowSetAlpha::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    if (!Engine::getSuitableWindow(i_engine->getWindowSystem(), i_param, &hwnd))
        return;

    if (m_alpha < 0) {    // remove all alpha
        for (std::list<yamy::platform::WindowHandle>::iterator i = i_engine->m_windowsWithAlpha.begin();
                i != i_engine->m_windowsWithAlpha.end(); ++ i) {
            i_engine->getWindowSystem()->setWindowLayered(*i, false);
            i_engine->getWindowSystem()->redrawWindow(*i);
        }
        i_engine->m_windowsWithAlpha.clear();
    } else {
        if (i_engine->getWindowSystem()->isWindowLayered(hwnd)) {    // remove alpha
            std::list<yamy::platform::WindowHandle>::iterator
            i = std::find(i_engine->m_windowsWithAlpha.begin(), i_engine->m_windowsWithAlpha.end(),
                          hwnd);
            if (i == i_engine->m_windowsWithAlpha.end())
                return;    // already layered by the application

            i_engine->m_windowsWithAlpha.erase(i);

            i_engine->getWindowSystem()->setWindowLayered(hwnd, false);
        } else {    // add alpha
            i_engine->getWindowSystem()->setWindowLayered(hwnd, true);
            int alpha = m_alpha % 101;
            if (!i_engine->getWindowSystem()->setLayeredWindowAttributes(hwnd, 0,
                                            (unsigned char)(255 * alpha / 100), LWA_ALPHA)) {
                Acquire a(&i_engine->m_log, 0);
                i_engine->m_log << "error: &WindowSetAlpha(" << alpha
                << ") failed for yamy::platform::WindowHandle: " << std::hex
                << reinterpret_cast<ULONG_PTR>(hwnd) << std::dec << std::endl;
                return;
            }
            i_engine->m_windowsWithAlpha.push_front(hwnd);
        }
        i_engine->getWindowSystem()->redrawWindow(hwnd);
    }
}

std::ostream &Command_WindowSetAlpha::outputArgs(std::ostream &i_ost) const
{
    i_ost << m_alpha;
    return i_ost;
}
