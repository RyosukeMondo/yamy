#include "cmd_mouse_move.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_MouseMove::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

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
    WindowPoint pt;
    i_engine->m_windowSystem->getCursorPos(&pt);
    i_engine->m_windowSystem->setCursorPos(pt.x + m_dx, pt.y + m_dy);
}

tostream &Command_MouseMove::outputArgs(tostream &i_ost) const
{
    i_ost << m_dx << _T(", ");
    i_ost << m_dy;
    return i_ost;
}
