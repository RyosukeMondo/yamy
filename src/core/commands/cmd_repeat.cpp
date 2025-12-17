#include "cmd_repeat.h"
#include "../engine/engine.h"
#include "../functions/function.h"
#include "../input/keymap.h"

// Specialization for loading KeySeq*
// The JSON loader handles key sequences directly in the mappings section.
// Also KeySeq ownership: FunctionData_Repeat deletes m_keySeq in destructor!
// So Command_Repeat must also own it.
// Tuple `std::tuple<KeySeq*, int>` will store the pointer.
// We need a destructor to delete it.

// Destructor to delete owned KeySeq
void Command_Repeat::exec(Engine *i_engine, FunctionParam *i_param) const
{
    // Logic from Engine::funcRepeat
    const KeySeq *keySeq = std::get<0>(m_args);
    int max = std::get<1>(m_args);

    if (i_param->m_isPressed) {
        int end = MIN(i_engine->m_variable, max);
        for (int i = 0; i < end - 1; ++ i)
            i_engine->generateKeySeqEvents(i_param->m_c, keySeq, Engine::Part_all);
        if (0 < end)
            i_engine->generateKeySeqEvents(i_param->m_c, keySeq, Engine::Part_down);
    } else
        i_engine->generateKeySeqEvents(i_param->m_c, keySeq, Engine::Part_up);
}
