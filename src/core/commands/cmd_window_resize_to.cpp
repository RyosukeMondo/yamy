#include "cmd_window_resize_to.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "../../platform/windows/windowstool.h" // For asyncResize

Command_WindowResizeTo::Command_WindowResizeTo()
{
    m_width = 0;
    m_height = 0;
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowResizeTo::load(SettingLoader *i_sl)
{
    std::string sName = getName();
    const char* cName = sName.c_str();

    i_sl->getOpenParen(true, cName); // throw ...
    i_sl->load_ARGUMENT(&m_width);
    i_sl->getComma(false, cName); // throw ...
    i_sl->load_ARGUMENT(&m_height);
    if (i_sl->getCloseParen(false, cName))
      return;
    i_sl->getComma(false, cName); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, cName); // throw ...
}

void Command_WindowResizeTo::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    yamy::platform::Rect rc, rcd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->getWindowSystem(), i_param, &hwnd, &twt, &rc, &rcd))
        return;
    asyncResize(static_cast<HWND>(hwnd), m_width, m_height);
}

tostream &Command_WindowResizeTo::outputArgs(tostream &i_ost) const
{
    i_ost << m_width << _T(", ");
    i_ost << m_height << _T(", ");
    i_ost << m_twt;
    return i_ost;
}
