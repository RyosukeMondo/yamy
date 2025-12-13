#include "cmd_clipboard_change_case.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_ClipboardChangeCase::load(SettingLoader *i_sl)
{
    const char* tName = Name;

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_doesConvertToUpperCase);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_ClipboardChangeCase::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    std::string text = i_engine->getWindowSystem()->getClipboardText();
    if (text.empty())
        return;

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        // Skip multi-byte UTF-8 continuation bytes
        if ((c & 0xC0) == 0x80) {
            continue;
        }
        // Only convert ASCII characters
        if (c >= 0 && c < 128) {
            text[i] = m_doesConvertToUpperCase ? toupper(c) : tolower(c);
        }
    }

    i_engine->getWindowSystem()->setClipboardText(text);
}

std::ostream &Command_ClipboardChangeCase::outputArgs(std::ostream &i_ost) const
{
    i_ost << m_doesConvertToUpperCase;
    return i_ost;
}
