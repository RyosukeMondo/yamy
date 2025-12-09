//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_modifier.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>
#include <process.h>


// is modifier pressed ?
bool Engine::isPressed(Modifier::Type i_mt)
{
    const Keymap::ModAssignments &ma = m_currentKeymap->getModAssignments(i_mt);
    for (Keymap::ModAssignments::const_iterator i = ma.begin();
            i != ma.end(); ++ i)
        if ((*i).m_key->m_isPressed)
            return true;
    return false;
}


// fix modifier key (if fixed, return true)
bool Engine::fixModifierKey(ModifiedKey *io_mkey, Keymap::AssignMode *o_am)
{
    // for all modifier ...
    for (int i = Modifier::Type_begin; i != Modifier::Type_end; ++ i) {
        // get modifier assignments (list of modifier keys)
        const Keymap::ModAssignments &ma =
            m_currentKeymap->getModAssignments(static_cast<Modifier::Type>(i));

        for (Keymap::ModAssignments::const_iterator
                j = ma.begin(); j != ma.end(); ++ j)
            if (io_mkey->m_key == (*j).m_key) { // is io_mkey a modifier ?
                {
                    Acquire a(&m_log, 1);
                    m_log << "* Modifier Key" << std::endl;
                }
                // set dontcare for this modifier
                io_mkey->m_modifier.dontcare(static_cast<Modifier::Type>(i));
                *o_am = (*j).m_assignMode;
                return true;
            }
    }
    *o_am = Keymap::AM_notModifier;
    return false;
}


// get current modifiers
Modifier Engine::getCurrentModifiers(Key *i_key, bool i_isPressed)
{
    Modifier cmods;
    cmods.add(m_currentLock);

    cmods.press(Modifier::Type_Shift  , isPressed(Modifier::Type_Shift  ));
    cmods.press(Modifier::Type_Alt    , isPressed(Modifier::Type_Alt    ));
    cmods.press(Modifier::Type_Control, isPressed(Modifier::Type_Control));
    cmods.press(Modifier::Type_Windows, isPressed(Modifier::Type_Windows));
    cmods.press(Modifier::Type_Up     , !i_isPressed);
    cmods.press(Modifier::Type_Down   , i_isPressed);

    cmods.press(Modifier::Type_Repeat , false);
    if (m_lastPressedKey[0] == i_key) {
        if (i_isPressed)
            cmods.press(Modifier::Type_Repeat, true);
        else
            if (m_lastPressedKey[1] == i_key)
                cmods.press(Modifier::Type_Repeat, true);
    }

    for (int i = Modifier::Type_Mod0; i <= Modifier::Type_Mod19; ++ i)
        cmods.press(static_cast<Modifier::Type>(i),
                    isPressed(static_cast<Modifier::Type>(i)));

    return cmods;
}
