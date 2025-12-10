//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>
#ifdef _WIN32
#include <process.h>
#endif


// keyboard handler thread
unsigned int WINAPI Engine::keyboardHandler(void *i_this)
{
    reinterpret_cast<Engine *>(i_this)->keyboardHandler();
    _endthreadex(0);
    return 0;
}
void Engine::keyboardHandler()
{
    // loop
    Key key;
    while (1) {
        KEYBOARD_INPUT_DATA kid;

#ifdef _WIN32
        WaitForSingleObject(m_queueMutex, INFINITE);
        while (SignalObjectAndWait(m_queueMutex, m_readEvent, INFINITE, true) == WAIT_OBJECT_0) {
#endif
            if (m_inputQueue == nullptr) {
                ReleaseMutex(m_queueMutex);
                return;
            }

            if (m_inputQueue->empty()) {
                ResetEvent(m_readEvent);
                continue;
            }

            kid = m_inputQueue->front();
            m_inputQueue->pop_front();
            if (m_inputQueue->empty()) {
                ResetEvent(m_readEvent);
            }

            break;

#if 0
            case WAIT_OBJECT_0 + NUMBER_OF(handles): {
#ifdef _WIN32
                MSG message;

                while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
#endif
                    switch (message.message) {
                    case WM_APP + 201: {
                        if (message.wParam) {
                            m_currentLock.on(Modifier::Type_Touchpad);
                            m_currentLock.on(Modifier::Type_TouchpadSticky);
                        } else
                            m_currentLock.off(Modifier::Type_Touchpad);
                        Acquire a(&m_log, 1);
                        m_log << "touchpad: " << message.wParam
                        << "." << (message.lParam & 0xffff)
                        << "." << (message.lParam >> 16 & 0xffff)
                        << std::endl;
                        break;
                    }
                    default:
                        break;
                    }
                }
#ifdef _WIN32
                goto rewait;
#endif
            }
#endif
        }
#ifdef _WIN32
        ReleaseMutex(m_queueMutex);
#endif

        checkFocusWindow();

        if (!m_setting ||    // m_setting has not been loaded
                !m_isEnabled) {    // disabled
            if (m_isLogMode) {
                Key key;
                key.addScanCode(ScanCode(kid.MakeCode, kid.Flags));
                outputToLog(&key, ModifiedKey(), 0);
                if (kid.Flags & KEYBOARD_INPUT_DATA::E1) {
                    // through mouse event even if log mode
                    injectInput(&kid, nullptr);
                }
            } else {
                injectInput(&kid, nullptr);
            }
            updateLastPressedKey(nullptr);
            continue;
        }

        Acquire a(&m_cs);

        if (!m_currentFocusOfThread ||
                !m_currentKeymap) {
            injectInput(&kid, nullptr);
            Acquire a(&m_log, 0);
            if (!m_currentFocusOfThread)
                m_log << "internal error: m_currentFocusOfThread == nullptr"
                << std::endl;
            if (!m_currentKeymap)
                m_log << "internal error: m_currentKeymap == nullptr"
                << std::endl;
            updateLastPressedKey(nullptr);
            continue;
        }

        Current c;
        c.m_keymap = m_currentKeymap;
        c.m_i = m_currentFocusOfThread->m_keymaps.begin();

        // search key
        Key mouseKey;
        Key *pProcessingKey = &key;
        bool isMouseEvent = (kid.ExtraInformation == 0x59414D59);
        
        if (isMouseEvent) {
            mouseKey.addScanCode(ScanCode(kid.MakeCode, kid.Flags));
            pProcessingKey = &mouseKey;
        } else {
            key.addScanCode(ScanCode(kid.MakeCode, kid.Flags));
        }

        c.m_mkey = m_setting->m_keyboard.searchKey(*pProcessingKey);
        if (!c.m_mkey.m_key) {
            if (!isMouseEvent) {
                c.m_mkey.m_key = m_setting->m_keyboard.searchPrefixKey(*pProcessingKey);
                if (c.m_mkey.m_key)
                    continue;
            }
        }

        // press the key and update counter
        bool isPhysicallyPressed
        = !(pProcessingKey->getScanCodes()[0].m_flags & ScanCode::BREAK);
        if (c.m_mkey.m_key) {
            if (!c.m_mkey.m_key->m_isPressed && isPhysicallyPressed)
                ++ m_currentKeyPressCount;
            else if (c.m_mkey.m_key->m_isPressed && !isPhysicallyPressed)
                -- m_currentKeyPressCount;
            c.m_mkey.m_key->m_isPressed = isPhysicallyPressed;
        }

        // create modifiers
        c.m_mkey.m_modifier = getCurrentModifiers(c.m_mkey.m_key,
                              isPhysicallyPressed);
        Keymap::AssignMode am;
        bool isModifier = fixModifierKey(&c.m_mkey, &am);
        if (m_isPrefix) {
            if (isModifier && m_doesIgnoreModifierForPrefix)
                am = Keymap::AM_true;
            if (m_doesEditNextModifier) {
                Modifier modifier = m_modifierForNextKey;
                modifier.add(c.m_mkey.m_modifier);
                c.m_mkey.m_modifier = modifier;
            }
        }

        if (m_isLogMode) {
            outputToLog(pProcessingKey, c.m_mkey, 0);
            if (kid.Flags & KEYBOARD_INPUT_DATA::E1) {
                // through mouse event even if log mode
                injectInput(&kid, nullptr);
            }
        } else if (am == Keymap::AM_true) {
            {
                Acquire a(&m_log, 1);
                m_log << "* true modifier" << std::endl;
            }
            // true modifier doesn't generate scan code
            outputToLog(pProcessingKey, c.m_mkey, 1);
        } else if (am == Keymap::AM_oneShot || am == Keymap::AM_oneShotRepeatable) {
            {
                Acquire a(&m_log, 1);
                if (am == Keymap::AM_oneShot)
                    m_log << "* one shot modifier" << std::endl;
                else
                    m_log << "* one shot repeatable modifier" << std::endl;
            }
            // oneShot modifier doesn't generate scan code
            outputToLog(pProcessingKey, c.m_mkey, 1);
            if (isPhysicallyPressed) {
                if (am == Keymap::AM_oneShotRepeatable    // the key is repeating
                        && m_oneShotKey.m_key == c.m_mkey.m_key) {
                    if (m_oneShotRepeatableRepeatCount <
                            m_setting->m_oneShotRepeatableDelay) {
                        ; // delay
                    } else {
                        Current cnew = c;
                        beginGeneratingKeyboardEvents(cnew, false);
                    }
                    ++ m_oneShotRepeatableRepeatCount;
                } else {
                    m_oneShotKey = c.m_mkey;
                    m_oneShotRepeatableRepeatCount = 0;
                }
            } else {
                if (m_oneShotKey.m_key) {
                    Current cnew = c;
                    cnew.m_mkey.m_modifier = m_oneShotKey.m_modifier;
                    cnew.m_mkey.m_modifier.off(Modifier::Type_Up);
                    cnew.m_mkey.m_modifier.on(Modifier::Type_Down);
                    beginGeneratingKeyboardEvents(cnew, false);

                    cnew = c;
                    cnew.m_mkey.m_modifier = m_oneShotKey.m_modifier;
                    cnew.m_mkey.m_modifier.on(Modifier::Type_Up);
                    cnew.m_mkey.m_modifier.off(Modifier::Type_Down);
                    beginGeneratingKeyboardEvents(cnew, false);
                }
                m_oneShotKey.m_key = nullptr;
                m_oneShotRepeatableRepeatCount = 0;
            }
        } else if (c.m_mkey.m_key) {
            // normal key
            outputToLog(pProcessingKey, c.m_mkey, 1);
            if (isPhysicallyPressed)
                m_oneShotKey.m_key = nullptr;
            beginGeneratingKeyboardEvents(c, isModifier);
        } else {
            // undefined key
            if (kid.Flags & KEYBOARD_INPUT_DATA::E1) {
                // through mouse event even if undefined for fail safe
                injectInput(&kid, nullptr);
            }
        }

        // if counter is zero, reset modifiers and keys on win32
        if (m_currentKeyPressCount <= 0) {
            {
                Acquire a(&m_log, 1);
                m_log << "* No key is pressed" << std::endl;
            }
            generateModifierEvents(Modifier());
            if (0 < m_currentKeyPressCountOnWin32)
                keyboardResetOnWin32();
            m_currentKeyPressCount = 0;
            m_currentKeyPressCountOnWin32 = 0;
            m_oneShotKey.m_key = nullptr;
            if (m_currentLock.isOn(Modifier::Type_Touchpad) == false)
                m_currentLock.off(Modifier::Type_TouchpadSticky);
        }

        if (!isMouseEvent)
            key.initialize();
        updateLastPressedKey(isPhysicallyPressed ? c.m_mkey.m_key : nullptr);
    }
}
