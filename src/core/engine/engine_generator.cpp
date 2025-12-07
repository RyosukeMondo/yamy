//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_generator.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>
#include <process.h>


// generate keyboard event for a key
void Engine::generateKeyEvent(Key *i_key, bool i_doPress, bool i_isByAssign)
{
    // check if key is event
    bool isEvent = false;
    for (Key **e = Event::events; *e; ++ e)
        if (*e == i_key) {
            isEvent = true;
            break;
        }

    bool isAlreadyReleased = false;

    if (!isEvent) {
        if (i_doPress && !i_key->m_isPressedOnWin32)
            ++ m_currentKeyPressCountOnWin32;
        else if (!i_doPress) {
            if (i_key->m_isPressedOnWin32)
                -- m_currentKeyPressCountOnWin32;
            else
                isAlreadyReleased = true;
        }
        i_key->m_isPressedOnWin32 = i_doPress;

        if (i_isByAssign)
            i_key->m_isPressedByAssign = i_doPress;

        Key *sync = m_setting->m_keyboard.getSyncKey();

        if (!isAlreadyReleased || i_key == sync) {
            KEYBOARD_INPUT_DATA kid = { 0, 0, 0, 0, 0 };
            const ScanCode *sc = i_key->getScanCodes();
            for (size_t i = 0; i < i_key->getScanCodesSize(); ++ i) {
                kid.MakeCode = sc[i].m_scan;
                kid.Flags = sc[i].m_flags;
                if (!i_doPress)
                    kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
                injectInput(&kid, nullptr);
            }

            m_lastGeneratedKey = i_doPress ? i_key : nullptr;
        }
    }

    {
        Acquire a(&m_log, 1);
        m_log << _T("\t\t    =>\t");
        if (isAlreadyReleased)
            m_log << _T("(already released) ");
    }
    ModifiedKey mkey(i_key);
    mkey.m_modifier.on(Modifier::Type_Up, !i_doPress);
    mkey.m_modifier.on(Modifier::Type_Down, i_doPress);
    outputToLog(i_key, mkey, 1);
}


// genete event
void Engine::generateEvents(Current i_c, const Keymap *i_keymap, Key *i_event)
{
    // generate
    i_c.m_keymap = i_keymap;
    i_c.m_mkey.m_key = i_event;
    if (const Keymap::KeyAssignment *keyAssign =
                i_c.m_keymap->searchAssignment(i_c.m_mkey)) {
        {
            Acquire a(&m_log, 1);
            m_log << std::endl << _T("           ")
            << i_event->getName() << std::endl;
        }
        generateKeySeqEvents(i_c, keyAssign->m_keySeq, Part_all);
    }
}


// genete modifier events
void Engine::generateModifierEvents(const Modifier &i_mod)
{
    {
        Acquire a(&m_log, 1);
        m_log << _T("* Gen Modifiers\t{") << std::endl;
    }

    for (int i = Modifier::Type_begin; i < Modifier::Type_BASIC; ++ i) {
        Keyboard::Mods &mods =
            m_setting->m_keyboard.getModifiers(static_cast<Modifier::Type>(i));

        if (i_mod.isDontcare(static_cast<Modifier::Type>(i)))
            // no need to process
            ;
        else if (i_mod.isPressed(static_cast<Modifier::Type>(i)))
            // we have to press this modifier
        {
            bool noneIsPressed = true;
            bool noneIsPressedByAssign = true;
            for (Keyboard::Mods::iterator i = mods.begin(); i != mods.end(); ++ i) {
                if ((*i)->m_isPressedOnWin32)
                    noneIsPressed = false;
                if ((*i)->m_isPressedByAssign)
                    noneIsPressedByAssign = false;
            }
            if (noneIsPressed) {
                if (noneIsPressedByAssign)
                    generateKeyEvent(mods.front(), true, false);
                else
                    for (Keyboard::Mods::iterator
                            i = mods.begin(); i != mods.end(); ++ i)
                        if ((*i)->m_isPressedByAssign)
                            generateKeyEvent((*i), true, false);
            }
        }

        else
            // we have to release this modifier
        {
            // avoid such sequences as  "Alt U-ALt" or "Windows U-Windows"
            if (i == Modifier::Type_Alt || i == Modifier::Type_Windows) {
                for (Keyboard::Mods::iterator j = mods.begin(); j != mods.end(); ++ j)
                    if ((*j) == m_lastGeneratedKey) {
                        Keyboard::Mods *mods =
                            &m_setting->m_keyboard.getModifiers(Modifier::Type_Shift);
                        if (mods->size() == 0)
                            mods = &m_setting->m_keyboard.getModifiers(
                                       Modifier::Type_Control);
                        if (0 < mods->size()) {
                            generateKeyEvent(mods->front(), true, false);
                            generateKeyEvent(mods->front(), false, false);
                        }
                        break;
                    }
            }

            for (Keyboard::Mods::iterator j = mods.begin(); j != mods.end(); ++ j) {
                if ((*j)->m_isPressedOnWin32)
                    generateKeyEvent((*j), false, false);
            }
        }
    }

    {
        Acquire a(&m_log, 1);
        m_log << _T("\t\t}") << std::endl;
    }
}


// generate keyboard events for action
void Engine::generateActionEvents(const Current &i_c, const Action *i_a,
                                  bool i_doPress)
{
    switch (i_a->getType()) {
        // key
    case Action::Type_key: {
        const ModifiedKey &mkey
        = reinterpret_cast<ActionKey *>(
              const_cast<Action *>(i_a))->m_modifiedKey;

        // release
        if (!i_doPress &&
                (mkey.m_modifier.isOn(Modifier::Type_Up) ||
                 mkey.m_modifier.isDontcare(Modifier::Type_Up)))
            generateKeyEvent(mkey.m_key, false, true);

        // press
        else if (i_doPress &&
                 (mkey.m_modifier.isOn(Modifier::Type_Down) ||
                  mkey.m_modifier.isDontcare(Modifier::Type_Down))) {
            Modifier modifier = mkey.m_modifier;
            modifier.add(i_c.m_mkey.m_modifier);
            generateModifierEvents(modifier);
            generateKeyEvent(mkey.m_key, true, true);
        }
        break;
    }

    // keyseq
    case Action::Type_keySeq: {
        const ActionKeySeq *aks = reinterpret_cast<const ActionKeySeq *>(i_a);
        generateKeySeqEvents(i_c, aks->m_keySeq,
                             i_doPress ? Part_down : Part_up);
        break;
    }

    // function
    case Action::Type_function: {
        const ActionFunction *af = reinterpret_cast<const ActionFunction *>(i_a);
        bool is_up = (!i_doPress &&
                      (af->m_modifier.isOn(Modifier::Type_Up) ||
                       af->m_modifier.isDontcare(Modifier::Type_Up)));
        bool is_down = (i_doPress &&
                        (af->m_modifier.isOn(Modifier::Type_Down) ||
                         af->m_modifier.isDontcare(Modifier::Type_Down)));

        if (!is_down && !is_up)
            break;

        {
            Acquire a(&m_log, 1);
            m_log << _T("\t\t     >\t") << af->m_functionData;
        }

        FunctionParam param;
        param.m_isPressed = i_doPress;
        param.m_hwnd = m_currentFocusOfThread->m_hwndFocus;
        param.m_c = i_c;
        param.m_doesNeedEndl = true;
        param.m_af = af;

        param.m_c.m_mkey.m_modifier.on(Modifier::Type_Up, !i_doPress);
        param.m_c.m_mkey.m_modifier.on(Modifier::Type_Down, i_doPress);

        af->m_functionData->exec(this, &param);

        if (param.m_doesNeedEndl) {
            Acquire a(&m_log, 1);
            m_log << std::endl;
        }
        break;
    }
    }
}


// generate keyboard events for keySeq
void Engine::generateKeySeqEvents(const Current &i_c, const KeySeq *i_keySeq,
                                  Part i_part)
{
    const KeySeq::Actions &actions = i_keySeq->getActions();
    if (actions.empty())
        return;
    if (i_part == Part_up)
        generateActionEvents(i_c, actions[actions.size() - 1].get(), false);
    else {
        size_t i;
        for (i = 0 ; i < actions.size() - 1; ++ i) {
            generateActionEvents(i_c, actions[i].get(), true);
            generateActionEvents(i_c, actions[i].get(), false);
        }
        generateActionEvents(i_c, actions[i].get(), true);
        if (i_part == Part_all)
            generateActionEvents(i_c, actions[i].get(), false);
    }
}


// generate keyboard events for current key
void Engine::generateKeyboardEvents(const Current &i_c)
{
    if (++ m_generateKeyboardEventsRecursionGuard ==
            MAX_GENERATE_KEYBOARD_EVENTS_RECURSION_COUNT) {
        Acquire a(&m_log);
        m_log << _T("error: too deep keymap recursion.  there may be a loop.")
        << std::endl;
        return;
    }

    const Keymap::KeyAssignment *keyAssign
    = i_c.m_keymap->searchAssignment(i_c.m_mkey);
    if (!keyAssign) {
        const KeySeq *keySeq = i_c.m_keymap->getDefaultKeySeq();
        ASSERT( keySeq );
        generateKeySeqEvents(i_c, keySeq, i_c.isPressed() ? Part_down : Part_up);
    } else {
        if (keyAssign->m_modifiedKey.m_modifier.isOn(Modifier::Type_Up) ||
                keyAssign->m_modifiedKey.m_modifier.isOn(Modifier::Type_Down))
            generateKeySeqEvents(i_c, keyAssign->m_keySeq, Part_all);
        else
            generateKeySeqEvents(i_c, keyAssign->m_keySeq,
                                 i_c.isPressed() ? Part_down : Part_up);
    }
    m_generateKeyboardEventsRecursionGuard --;
}


// generate keyboard events for current key
void Engine::beginGeneratingKeyboardEvents(
    const Current &i_c, bool i_isModifier)
{
    //             (1)             (2)             (3)  (4)   (1)
    // up/down:    D-              U-              D-   U-    D-
    // keymap:     m_currentKeymap m_currentKeymap X    X     m_currentKeymap
    // memo:       &Prefix(X)      ...             ...  ...   ...
    // m_isPrefix: false           true            true false false

    Current cnew(i_c);

    bool isPhysicallyPressed
    = cnew.m_mkey.m_modifier.isPressed(Modifier::Type_Down);

    // substitute
    ModifiedKey mkey = m_setting->m_keyboard.searchSubstitute(cnew.m_mkey);
    if (mkey.m_key) {
        cnew.m_mkey = mkey;
        if (isPhysicallyPressed) {
            cnew.m_mkey.m_modifier.off(Modifier::Type_Up);
            cnew.m_mkey.m_modifier.on(Modifier::Type_Down);
        } else {
            cnew.m_mkey.m_modifier.on(Modifier::Type_Up);
            cnew.m_mkey.m_modifier.off(Modifier::Type_Down);
        }
        for (int i = Modifier::Type_begin; i != Modifier::Type_end; ++ i) {
            Modifier::Type type = static_cast<Modifier::Type>(i);
            if (cnew.m_mkey.m_modifier.isDontcare(type) &&
                    !i_c.m_mkey.m_modifier.isDontcare(type))
                cnew.m_mkey.m_modifier.press(
                    type, i_c.m_mkey.m_modifier.isPressed(type));
        }

        {
            Acquire a(&m_log, 1);
            m_log << _T("* substitute") << std::endl;
        }
        outputToLog(mkey.m_key, cnew.m_mkey, 1);
    }

    // for prefix key
    const Keymap *tmpKeymap = m_currentKeymap;
    if (i_isModifier || !m_isPrefix) ;
    else if (isPhysicallyPressed)            // when (3)
        m_isPrefix = false;
    else if (!isPhysicallyPressed)        // when (2)
        m_currentKeymap = m_currentFocusOfThread->m_keymaps.front();

    // for m_emacsEditKillLine function
    m_emacsEditKillLine.m_doForceReset = !i_isModifier;

    // generate key event !
    m_generateKeyboardEventsRecursionGuard = 0;
    if (isPhysicallyPressed)
        generateEvents(cnew, cnew.m_keymap, &Event::before_key_down);
    generateKeyboardEvents(cnew);
    if (!isPhysicallyPressed)
        generateEvents(cnew, cnew.m_keymap, &Event::after_key_up);

    // for m_emacsEditKillLine function
    if (m_emacsEditKillLine.m_doForceReset)
        m_emacsEditKillLine.reset();

    // for prefix key
    if (i_isModifier)
        ;
    else if (!m_isPrefix)                // when (1), (4)
        m_currentKeymap = m_currentFocusOfThread->m_keymaps.front();
    else if (!isPhysicallyPressed)        // when (2)
        m_currentKeymap = tmpKeymap;
}
