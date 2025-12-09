#include "cmd_clipboard_downcase_word.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators
#include "cmd_clipboard_change_case.h" // Reuse logic

void Command_ClipboardDowncaseWord::exec(Engine *i_engine, FunctionParam *i_param) const
{
    Command_ClipboardChangeCase cmd;
    cmd.m_doesConvertToUpperCase = BooleanType_false;
    cmd.exec(i_engine, i_param);
}
