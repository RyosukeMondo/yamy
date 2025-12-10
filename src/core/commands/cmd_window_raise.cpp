#include "cmd_window_raise.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

Command_WindowRaise::Command_WindowRaise()
{
    m_twt = TargetWindowType_overlapped;
}

void Command_WindowRaise::load(SettingLoader *i_sl)
{
    std::string sName = getName();
    const char* cName = sName.c_str();

    if (!i_sl->getOpenParen(false, cName))
      return;
    if (i_sl->getCloseParen(false, cName))
      return;
    i_sl->load_ARGUMENT(&m_twt);
    i_sl->getCloseParen(true, cName); // throw ...
}

void Command_WindowRaise::exec(Engine *i_engine, FunctionParam *i_param) const
{
    yamy::platform::WindowHandle hwnd;
    TargetWindowType twt = m_twt;
    if (!Engine::getSuitableMdiWindow(i_engine->getWindowSystem(), i_param, &hwnd, &twt))
        return;
    i_engine->getWindowSystem()->setWindowZOrder(hwnd, yamy::platform::ZOrder::Top);
}

tostream &Command_WindowRaise::outputArgs(tostream &i_ost) const
{
    i_ost << m_twt;
    return i_ost;
}
