#include "cmd_window_resize_to.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowResizeTo::Command_WindowResizeTo()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowResizeTo::load(SettingLoader *i_sl)
{
    i_sl->getOpenParen(true, Name); // throw ...
    i_sl->load_ARGUMENT(&m_width);
    i_sl->getComma(false, Name); // throw ...
    i_sl->load_ARGUMENT(&m_height);
    if (i_sl->getCloseParen(false, Name))
      return;
    i_sl->getComma(false, Name); // throw ...
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, Name); // throw ...
}

void Command_WindowResizeTo::exec(Engine *i_engine, FunctionParam *i_param) const
{
    HWND hwnd;
    RECT rc, rcd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->m_windowSystem, i_param, &hwnd, &twt, &rc, &rcd))
        return;

    int width = m_width;
    int height = m_height;

    if (width == 0)
        width = rcWidth(&rc);
    else if (width < 0)
        width += rcWidth(&rcd);

    if (height == 0)
        height = rcHeight(&rc);
    else if (height < 0)
        height += rcHeight(&rcd);

    i_engine->asyncResize(hwnd, width, height);
}

tostream &Command_WindowResizeTo::outputArgs(tostream &i_ost) const
{
    i_ost << m_width << _T(", ");
    i_ost << m_height << _T(", ");
    i_ost << m_twt;
    return i_ost;
}
