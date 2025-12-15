//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_modifier.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>
#include <gsl/gsl>


bool Engine::isPressed(Modifier::Type i_mt)
{
    // For modal modifiers (mod0-mod19), check ModifierState instead of physical key press
    // Modal modifiers are activated by EventProcessor and stored in m_modifierState
    if (i_mt >= Modifier::Type_Mod0 && i_mt <= Modifier::Type_Mod19) {
        bool active = m_modifierState.isActive(i_mt);
        if (active) {
            Acquire a(&m_log, 0);
            m_log << "[DEBUG] isPressed: mod" << (i_mt - Modifier::Type_Mod0) << " = ACTIVE" << std::endl;
        }
        return active;
    }

    // For hardware modifiers, check physical key press state
    const Keymap::ModAssignments &ma = m_currentKeymap->getModAssignments(i_mt);
    for (Keymap::ModAssignments::const_iterator i = ma.begin();
            i != ma.end(); ++ i)
        if ((*i).m_key->m_isPressed)
            return true;
    return false;
}


bool Engine::fixModifierKey(ModifiedKey *io_mkey, Keymap::AssignMode *o_am)
{
    for (int i = Modifier::Type_begin; i != Modifier::Type_end; ++ i) {
        const Keymap::ModAssignments &ma =
            m_currentKeymap->getModAssignments(static_cast<Modifier::Type>(i));

        for (Keymap::ModAssignments::const_iterator
                j = ma.begin(); j != ma.end(); ++ j)
            if (io_mkey->m_key == (*j).m_key) {
                {
                    Acquire a(&m_log, 1);
                    m_log << "* Modifier Key" << std::endl;
                }
                io_mkey->m_modifier.dontcare(static_cast<Modifier::Type>(i));
                *o_am = (*j).m_assignMode;
                Ensures(*o_am >= Keymap::AM_normal && *o_am <= Keymap::AM_oneShotRepeatable);
                return true;
            }
    }
    *o_am = Keymap::AM_notModifier;
    Ensures(*o_am == Keymap::AM_notModifier);
    return false;
}


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

    Ensures(cmods.isPressed(Modifier::Type_Up) != cmods.isPressed(Modifier::Type_Down));
    return cmods;
}
