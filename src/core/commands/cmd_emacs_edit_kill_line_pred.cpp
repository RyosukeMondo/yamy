#include "cmd_emacs_edit_kill_line_pred.h"
#include "../engine/engine.h"
#include "../functions/function.h" // For type tables and ToString operators

void Command_EmacsEditKillLinePred::load(SettingLoader *i_sl)
{
    tstring tsName = to_tstring(Name);
    const _TCHAR* tName = tsName.c_str();

    i_sl->getOpenParen(true, tName); // throw ...
    i_sl->load_ARGUMENT(&m_keySeq1);
    i_sl->getComma(false, tName); // throw ...
    i_sl->load_ARGUMENT(&m_keySeq2);
    i_sl->getCloseParen(true, tName); // throw ...
}

void Command_EmacsEditKillLinePred::exec(Engine *i_engine, FunctionParam *i_param) const
{
    i_engine->m_emacsEditKillLine.m_doForceReset = false;
    if (!i_param->m_isPressed)
        return;

    int r = i_engine->m_emacsEditKillLine.pred(i_engine->getWindowSystem());
    const KeySeq *keySeq;
    if (r == 1)
        keySeq = m_keySeq1;
    else if (r == 2)
        keySeq = m_keySeq2;
    else // r == 0
        return;
    ASSERT(keySeq);
    i_engine->generateKeySeqEvents(i_param->m_c, keySeq, Engine::Part_all);
}

tostream &Command_EmacsEditKillLinePred::outputArgs(tostream &i_ost) const
{
    i_ost << m_keySeq1 << _T(", ");
    i_ost << m_keySeq2;
    return i_ost;
}
