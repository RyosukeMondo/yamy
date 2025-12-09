#include "cmd_clipboard_copy.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_ClipboardCopy::load(SettingLoader *i_sl)
{
    i_sl->getOpenParen(true, Name); // throw ...
    i_sl->load_ARGUMENT(&m_text);
    i_sl->getCloseParen(true, Name); // throw ...
}

void Command_ClipboardCopy::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    i_engine->m_windowSystem->setClipboardText(m_text.eval());
}

tostream &Command_ClipboardCopy::outputArgs(tostream &i_ost) const
{
    i_ost << m_text;
    return i_ost;
}
