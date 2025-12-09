#include "cmd_clipboard_change_case.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_ClipboardChangeCase::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_doesConvertToUpperCase);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_ClipboardChangeCase::exec(Engine *i_engine, FunctionParam *i_param) const
{
    if (!i_param->m_isPressed)
        return;

    tstring text = to_tstring(i_engine->getWindowSystem()->getClipboardText());
    if (text.empty())
        return;

    for (size_t i = 0; i < text.size(); ++i) {
        _TCHAR c = text[i];
#ifndef _UNICODE
        if (_istlead(c)) {
            ++i; // Skip next char as it is trail byte
            continue;
        }
#endif
        text[i] = m_doesConvertToUpperCase ? _totupper(c) : _totlower(c);
    }

    i_engine->getWindowSystem()->setClipboardText(to_UTF_8(text));
}

tostream &Command_ClipboardChangeCase::outputArgs(tostream &i_ost) const
{
    i_ost << m_doesConvertToUpperCase;
    return i_ost;
}
