#include "cmd_mouse_move.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_MouseMove::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_dx);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_dy);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_MouseMove::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;
    yamy::platform::Point pt;
    i_engine->getWindowSystem()->getCursorPos(&pt);
    yamy::platform::Point newPt;
    newPt.x = pt.x + m_dx;
    newPt.y = pt.y + m_dy;
    i_engine->getWindowSystem()->setCursorPos(newPt);
}

std::ostream &Command_MouseMove::outputArgs(std::ostream &i_ost) const
{
    i_ost << m_dx << ", ";
    i_ost << m_dy;
    return i_ost;
}
