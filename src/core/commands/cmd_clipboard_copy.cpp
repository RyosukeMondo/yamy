#include "cmd_clipboard_copy.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_ClipboardCopy::load(SettingLoader *i_sl)
{
    std::string sName = getName();
    const char* cName = sName.c_str();

    i_sl->getOpenParen(true, cName); // throw ...
    i_sl->load_ARGUMENT(&m_text);
    i_sl->getCloseParen(true, cName); // throw ...
}

void Command_ClipboardCopy::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    i_engine->getWindowSystem()->setClipboardText(to_UTF_8(to_tstring(m_text.eval())));
}

tostream &Command_ClipboardCopy::outputArgs(tostream &i_ost) const
{
    i_ost << m_text;
    return i_ost;
}
