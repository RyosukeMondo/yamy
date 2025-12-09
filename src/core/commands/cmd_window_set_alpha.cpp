#include "cmd_window_set_alpha.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include <algorithm>

void Command_WindowSetAlpha::load(SettingLoader *i_sl)
{
    i_sl->getOpenParen(true, Name); // throw ...
    i_sl->load_ARGUMENT(&m_alpha);
    i_sl->getCloseParen(true, Name); // throw ...
}

void Command_WindowSetAlpha::exec(Engine *i_engine, FunctionParam *i_param) const
{
    HWND hwnd;
    if (!Engine::getSuitableWindow(i_param, &hwnd))
        return;

    if (m_alpha < 0) {    // remove all alpha
        for (std::list<HWND>::iterator i = i_engine->m_windowsWithAlpha.begin();
                i != i_engine->m_windowsWithAlpha.end(); ++ i) {
            i_engine->m_windowSystem->setWindowLayered((WindowSystem::WindowHandle)*i, false);
            i_engine->m_windowSystem->redrawWindow((WindowSystem::WindowHandle)*i);
        }
        i_engine->m_windowsWithAlpha.clear();
    } else {
        if (i_engine->m_windowSystem->isWindowLayered((WindowSystem::WindowHandle)hwnd)) {    // remove alpha
            std::list<HWND>::iterator
            i = std::find(i_engine->m_windowsWithAlpha.begin(), i_engine->m_windowsWithAlpha.end(),
                          hwnd);
            if (i == i_engine->m_windowsWithAlpha.end())
                return;    // already layered by the application

            i_engine->m_windowsWithAlpha.erase(i);

            i_engine->m_windowSystem->setWindowLayered((WindowSystem::WindowHandle)hwnd, false);
        } else {    // add alpha
            i_engine->m_windowSystem->setWindowLayered((WindowSystem::WindowHandle)hwnd, true);
            int alpha = m_alpha % 101;
            if (!i_engine->m_windowSystem->setLayeredWindowAttributes((WindowSystem::WindowHandle)hwnd, 0,
                                            (unsigned char)(255 * alpha / 100), LWA_ALPHA)) {
                Acquire a(&i_engine->m_log, 0);
                i_engine->m_log << _T("error: &WindowSetAlpha(") << alpha
                << _T(") failed for HWND: ") << std::hex
                << reinterpret_cast<ULONG_PTR>(hwnd) << std::dec << std::endl;
                return;
            }
            i_engine->m_windowsWithAlpha.push_front(hwnd);
        }
        i_engine->m_windowSystem->redrawWindow((WindowSystem::WindowHandle)hwnd);
    }
}

tostream &Command_WindowSetAlpha::outputArgs(tostream &i_ost) const
{
    i_ost << m_alpha;
    return i_ost;
}
