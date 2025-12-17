//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_generator.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "stringtool.h"
#include "windowstool.h"
#include "../../utils/platform_logger.h"

#include <iomanip>


void Engine::generateKeyEvent(Key *i_key, bool i_doPress, bool i_isByAssign)
{
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
        m_log << "\t\t    =>\t";
        if (isAlreadyReleased)
            m_log << "(already released) ";
    }
    ModifiedKey mkey(i_key);
    mkey.m_modifier.on(Modifier::Type_Up, !i_doPress);
    mkey.m_modifier.on(Modifier::Type_Down, i_doPress);
    outputToLog(i_key, mkey, 1);
}


void Engine::generateEvents(Current i_c, const Keymap *i_keymap, Key *i_event)
{
    i_c.m_keymap = i_keymap;
    i_c.m_mkey.m_key = i_event;
    if (const Keymap::KeyAssignment *keyAssign =
                i_c.m_keymap->searchAssignment(i_c.m_mkey)) {
        {
            Acquire a(&m_log, 1);
            m_log << std::endl << "           "
            << to_tstring(i_event->getName()) << std::endl;
        }
        generateKeySeqEvents(i_c, keyAssign->m_keySeq, Part_all);
    }
}


void Engine::generateModifierEvents(const Modifier &i_mod)
{
    {
        Acquire a(&m_log, 1);
        m_log << "* Gen Modifiers\t{" << std::endl;
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
        m_log << "\t\t}" << std::endl;
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
            m_log << "\t\t     >\t" << af->m_functionData;
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
        m_log << "error: too deep keymap recursion.  there may be a loop."
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


// Check if any virtual modifiers (M00-MFF) are active
bool Engine::hasActiveVirtualModifiers() const
{
    const auto& state = m_modifierState.getFullState();
    for (int i = 0; i < 256; ++i) {
        if (state.test(yamy::input::ModifierState::VIRTUAL_OFFSET + i)) {
            return true;
        }
    }
    return false;
}

// Build ModifiedKey with physical key + active modifiers (before substitution)
ModifiedKey Engine::buildPhysicalModifiedKey(const Current& i_c) const
{
    ModifiedKey mkey;
    mkey.m_key = i_c.m_mkey.m_key;  // Physical key (before substitution)

    // Copy active virtual modifiers from ModifierState
    std::memset(mkey.m_virtualMods, 0, sizeof(mkey.m_virtualMods));
    const auto& state = m_modifierState.getFullState();
    for (int i = 0; i < 256; ++i) {
        if (state.test(yamy::input::ModifierState::VIRTUAL_OFFSET + i)) {
            mkey.m_virtualMods[i / 32] |= (1U << (i % 32));
        }
    }

    // Copy standard modifiers (Shift, Ctrl, Alt, Win, etc.)
    // Use getCurrentModifiers to get the current state
    bool isPressed = i_c.m_mkey.m_modifier.isPressed(Modifier::Type_Down);
    Modifier currentMods = const_cast<Engine*>(this)->getCurrentModifiers(mkey.m_key, isPressed);
    mkey.m_modifier = currentMods;

    return mkey;
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

    // NEW: Early keymap check if virtual modifiers are active
    // This ensures M20-*W matches PHYSICAL W, not substituted A
    // FIX: Only check for NON-MODIFIER keys. Modifier keys (like B/M20) must go through
    // EventProcessor to update ModifierState!
    if (hasActiveVirtualModifiers() && !i_isModifier) {
        // Build ModifiedKey with PHYSICAL key + active modifiers
        ModifiedKey physical_mkey = buildPhysicalModifiedKey(i_c);

        // Try to find keymap match with physical key
        const Keymap::KeyAssignment *keyAssign =
            m_currentKeymap->searchAssignment(physical_mkey);

        if (keyAssign) {
            // MATCH FOUND! Execute action and skip substitution
            
            // Generate the key sequence events
            generateKeySeqEvents(i_c, keyAssign->m_keySeq,
                                isPhysicallyPressed ? Part_down : Part_up);

            return;  // Done, skip rest of processing (including substitution)
        }
    }

    // FULL 3-LAYER EVENT PROCESSING via EventProcessor
    // Call EventProcessor::processEvent() for complete Layer1→Layer2→Layer3 flow
    if (m_eventProcessor && i_c.m_evdev_code != 0) {
        // Determine event type from modifier state
        yamy::EventType event_type = isPhysicallyPressed ? yamy::EventType::PRESS : yamy::EventType::RELEASE;

        // Process through all 3 layers
        // Pass ModifierState to track and update modal modifier state (mod0-mod19)
        // LockState is now integrated into ModifierState
        yamy::EventProcessor::ProcessedEvent result = m_eventProcessor->processEvent(i_c.m_evdev_code, event_type, &m_modifierState);

        // CRITICAL: Check if event was suppressed at Layer 3 (virtual key, lock key, etc.)
        // If output_evdev is 0, the event should NOT be generated/output
        if (result.output_evdev == 0) {
            return;  // Early return - don't generate any keyboard output
        }

        // SPECIAL HANDLING: TAP events need PRESS+RELEASE output
        // When a virtual modifier TAP is detected on RELEASE, we need to synthesize
        // a complete PRESS→RELEASE sequence for the tap action key
        if (result.is_tap && result.valid && result.output_yamy != 0) {
            // Find the Key object for the tap output
            Key* tap_key = nullptr;
            for (Keyboard::KeyIterator it = m_setting->m_keyboard.getKeyIterator(); *it; ++it) {
                const ScanCode *sc = (*it)->getScanCodes();
                if ((*it)->getScanCodesSize() > 0 && sc[0].m_scan == result.output_yamy) {
                    tap_key = *it;
                    break;
                }
            }

            if (tap_key) {
                // Generate PRESS event
                generateKeyEvent(tap_key, true, false);

                // Generate RELEASE event
                generateKeyEvent(tap_key, false, false);
            }

            // TAP event handled, return early
            return;
        }

        if (result.valid && result.output_yamy != 0) {
            // Get the original YAMY scan code for comparison
            const ScanCode *input_sc = cnew.m_mkey.m_key ? cnew.m_mkey.m_key->getScanCodes() : nullptr;
            uint16_t input_yamy = (input_sc && cnew.m_mkey.m_key->getScanCodesSize() > 0) ? input_sc[0].m_scan : 0;

            // Check if substitution occurred (output differs from input)
            if (result.output_yamy != input_yamy) {
                // Find the key object for the substituted YAMY scan code
                Key* substituted_key = nullptr;
                for (Keyboard::KeyIterator it = m_setting->m_keyboard.getKeyIterator(); *it; ++it) {
                    const ScanCode *sc = (*it)->getScanCodes();
                    if ((*it)->getScanCodesSize() > 0 && sc[0].m_scan == result.output_yamy) {
                        substituted_key = *it;
                        break;
                    }
                }

                if (substituted_key) {
                    ModifiedKey mkey(substituted_key);
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
                        m_log << "* substitute (via EventProcessor 3-layer)" << std::endl;
                    }
                    outputToLog(substituted_key, cnew.m_mkey, 1);
                }
            }
            // If no substitution (input == output), passthrough - no change to cnew.m_mkey
        }

        // CRITICAL FIX: We processed via EventProcessor. Now inject the key directly
        // and DO NOT fall through to generateKeyboardEvents which would run the old keymap logic!
        Key* output_key = cnew.m_mkey.m_key;
        if (output_key) {
            generateKeyEvent(output_key, isPhysicallyPressed, false);
        }
        return; // Skip old keymap logic entirely
    } else {
        // No EventProcessor and no substitution table - use old logic
        // Layer 2: Log input to substitution lookup
        if (cnew.m_mkey.m_key && cnew.m_mkey.m_key->getScanCodesSize() > 0) {
            const ScanCode *sc = cnew.m_mkey.m_key->getScanCodes();
            PLATFORM_LOG_INFO("Layer2", "[LAYER2:IN] Processing yamy 0x%04X", sc[0].m_scan);
        }

        // substitute
        ModifiedKey mkey = m_setting->m_keyboard.searchSubstitute(cnew.m_mkey);
        if (mkey.m_key) {
            // Layer 2: Log substitution occurred
            if (cnew.m_mkey.m_key && cnew.m_mkey.m_key->getScanCodesSize() > 0 &&
                mkey.m_key && mkey.m_key->getScanCodesSize() > 0) {
                const ScanCode *input_sc = cnew.m_mkey.m_key->getScanCodes();
                const ScanCode *output_sc = mkey.m_key->getScanCodes();
                PLATFORM_LOG_INFO("Layer2", "[LAYER2:SUBST] 0x%04X -> 0x%04X",
                    input_sc[0].m_scan, output_sc[0].m_scan);
            }

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
                m_log << "* substitute" << std::endl;
            }
            outputToLog(mkey.m_key, cnew.m_mkey, 1);
        } else {
            // Layer 2: Log passthrough (no substitution)
            if (cnew.m_mkey.m_key && cnew.m_mkey.m_key->getScanCodesSize() > 0) {
                const ScanCode *sc = cnew.m_mkey.m_key->getScanCodes();
                PLATFORM_LOG_INFO("Layer2", "[LAYER2:PASSTHROUGH] 0x%04X (no substitution)",
                    sc[0].m_scan);
            }
        }
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

    // Apply current modifier state from ModifierState to ModifiedKey
    // OLD system: Convert to Modifier and add (mod0-mod19)
    Modifier activeModifiers = m_modifierState.toModifier();
    cnew.m_mkey.m_modifier.add(activeModifiers);

    // NEW M00-MFF system: Copy virtual modifiers directly to new field
    std::memset(cnew.m_mkey.m_virtualMods, 0, sizeof(cnew.m_mkey.m_virtualMods));
    const auto& state = m_modifierState.getFullState();
    for (int i = 0; i < 256; ++i) {
        if (state.test(yamy::input::ModifierState::VIRTUAL_OFFSET + i)) {
            cnew.m_mkey.m_virtualMods[i / 32] |= (1U << (i % 32));
        }
    }

    std::cerr << "[GEN] Applied active modifiers: OLD=" << activeModifiers
              << ", NEW_M00=" << (state.test(yamy::input::ModifierState::VIRTUAL_OFFSET) ? "1" : "0") << std::endl;

    // generate key event !
    std::cerr << "[GEN] About to generate key events for cnew.m_mkey.m_key="
              << (cnew.m_mkey.m_key ? cnew.m_mkey.m_key->getName().c_str() : "NULL")
              << ", isPhysicallyPressed=" << isPhysicallyPressed << std::endl;

    m_generateKeyboardEventsRecursionGuard = 0;
    if (isPhysicallyPressed)
        generateEvents(cnew, cnew.m_keymap, &Event::before_key_down);

    std::cerr << "[GEN] Calling generateKeyboardEvents..." << std::endl;
    generateKeyboardEvents(cnew);
    std::cerr << "[GEN] generateKeyboardEvents returned" << std::endl;
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
