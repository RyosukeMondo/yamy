//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_keyboard_handler.cpp - Keyboard event handling thread

#include "misc.h"
#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "../platform/sync.h"
#include "core/logging/logger.h"
#include "../../utils/metrics.h"

#ifdef _WIN32
// keyboard handler thread - Windows static entry point
void* Engine::keyboardHandler(void *i_this)
{
    reinterpret_cast<Engine *>(i_this)->keyboardHandler();
    return nullptr;
}

void Engine::keyboardHandler()
{
    Key key;
    while (1) {
        yamy::platform::KeyEvent event;

        yamy::platform::waitForObject(m_queueMutex, yamy::platform::WAIT_INFINITE);
        while (SignalObjectAndWait(m_queueMutex, m_readEvent, INFINITE, true) == WAIT_OBJECT_0) {
            if (m_inputQueue == nullptr) {
                ReleaseMutex(m_queueMutex);
                return;
            }

            if (m_inputQueue->empty()) {
                ResetEvent(m_readEvent);
                continue;
            }

            event = m_inputQueue->front();
            m_inputQueue->pop_front();
            if (m_inputQueue->empty()) {
                ResetEvent(m_readEvent);
            }

            break;
        }
        ReleaseMutex(m_queueMutex);

        yamy::logging::Logger::getInstance().log(
            yamy::logging::LogLevel::Trace, "Engine",
            "Processing key event: scancode=" + std::to_string(event.scanCode) +
                ", isKeyDown=" + std::to_string(event.isKeyDown));
        auto keyProcessingStart = std::chrono::high_resolution_clock::now();

        KEYBOARD_INPUT_DATA kid = keyEventToKID(event);
        bool isPhysicallyPressed = event.isKeyDown;

        if (!m_setting || !m_isEnabled) {
            if (m_isLogMode) {
                Key logKey;
                logKey.addScanCode(ScanCode(kid.MakeCode, kid.Flags));
                outputToLog(&logKey, ModifiedKey(), 0);
                if (kid.Flags & KEYBOARD_INPUT_DATA::E1) {
                    injectInput(&kid, nullptr);
                }
            } else {
                injectInput(&kid, nullptr);
            }
            updateLastPressedKey(nullptr);
            continue;
        }

        Acquire a(&m_cs);

        if (!m_currentKeymap) {
            injectInput(&kid, nullptr);
            Acquire b(&m_log, 0);
            m_log << "internal error: m_currentKeymap == nullptr"
                << std::endl;
            updateLastPressedKey(nullptr);
            continue;
        }

        Current c;
        c.m_keymap = m_currentKeymap;
        c.m_evdev_code = static_cast<uint16_t>(event.scanCode);

        const uint32_t MOUSE_EVENT_MARKER = 0x59414D59;
        bool isMouseEvent = (event.extraInfo == MOUSE_EVENT_MARKER);

        // IMPORTANT: Clear key object for each new event to avoid accumulation
        key = Key();
        Key mouseKey;
        Key *pProcessingKey = &key;

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

        if (c.m_mkey.m_key) {
            if (!c.m_mkey.m_key->m_isPressed && isPhysicallyPressed)
                ++ m_currentKeyPressCount;
            else if (c.m_mkey.m_key->m_isPressed && !isPhysicallyPressed)
                -- m_currentKeyPressCount;
            c.m_mkey.m_key->m_isPressed = isPhysicallyPressed;
        }

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
                injectInput(&kid, nullptr);
            }
        } else if (am == Keymap::AM_true) {
            {
                Acquire b(&m_log, 1);
                m_log << "* true modifier" << std::endl;
            }
            outputToLog(pProcessingKey, c.m_mkey, 1);
        } else if (am == Keymap::AM_oneShot || am == Keymap::AM_oneShotRepeatable) {
            {
                Acquire b(&m_log, 1);
                if (am == Keymap::AM_oneShot)
                    m_log << "* one shot modifier" << std::endl;
                else
                    m_log << "* one shot repeatable modifier" << std::endl;
            }
            outputToLog(pProcessingKey, c.m_mkey, 1);
            if (isPhysicallyPressed) {
                if (am == Keymap::AM_oneShotRepeatable &&
                        m_oneShotKey.m_key == c.m_mkey.m_key) {
                    if (m_oneShotRepeatableRepeatCount <
                            m_setting->m_oneShotRepeatableDelay) {
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
            outputToLog(pProcessingKey, c.m_mkey, 1);
            if (isPhysicallyPressed)
                m_oneShotKey.m_key = nullptr;
            beginGeneratingKeyboardEvents(c, isModifier);
        } else {
            if (kid.Flags & KEYBOARD_INPUT_DATA::E1) {
                injectInput(&kid, nullptr);
            }
        }

        if (m_currentKeyPressCount <= 0) {
            {
                Acquire b(&m_log, 1);
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

        auto keyProcessingEnd = std::chrono::high_resolution_clock::now();
        auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            keyProcessingEnd - keyProcessingStart).count();
        yamy::metrics::PerformanceMetrics::instance().recordLatency(
            yamy::metrics::Operations::KEY_PROCESSING, static_cast<uint64_t>(durationNs));
    }
}

#else

void* Engine::keyboardHandler(void *i_this)
{
    reinterpret_cast<Engine *>(i_this)->keyboardHandler();
    return nullptr;
}

void Engine::keyboardHandler()
{
    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine",
        "Keyboard handler thread started, waiting for events...");

    Key key;
    while (1) {
        yamy::platform::KeyEvent event;

        yamy::platform::acquireMutex(m_queueMutex, yamy::platform::WAIT_INFINITE);

        while (true) {
            if (m_inputQueue == nullptr) {
                yamy::platform::releaseMutex(m_queueMutex);
                return;
            }

            if (m_inputQueue->empty()) {
                yamy::platform::resetEvent(m_readEvent);
                yamy::platform::releaseMutex(m_queueMutex);
                if (yamy::platform::waitForObject(m_readEvent, yamy::platform::WAIT_INFINITE) != yamy::platform::WaitResult::Success) {
                    yamy::platform::acquireMutex(m_queueMutex, yamy::platform::WAIT_INFINITE);
                    continue;
                }
                yamy::platform::acquireMutex(m_queueMutex, yamy::platform::WAIT_INFINITE);
                continue;
            }

            event = m_inputQueue->front();
            m_inputQueue->pop_front();
            if (m_inputQueue->empty()) {
                yamy::platform::resetEvent(m_readEvent);
            }

            break;
        }
        yamy::platform::releaseMutex(m_queueMutex);

        auto keyProcessingStart = std::chrono::high_resolution_clock::now();

        KEYBOARD_INPUT_DATA kid = keyEventToKID(event);
        std::cerr << "[HANDLER:DEBUG] Processing event: scan=" << std::hex << kid.MakeCode << std::dec << std::endl;
        bool isPhysicallyPressed = event.isKeyDown;

        if (!m_setting || !m_isEnabled) {
            if (m_isLogMode) {
                Key logKey;
                logKey.addScanCode(ScanCode(kid.MakeCode, kid.Flags));
                outputToLog(&logKey, ModifiedKey(), 0);
                if (kid.Flags & KEYBOARD_INPUT_DATA::E1) {
                    injectInput(&kid, nullptr);
                }
            } else {
                injectInput(&kid, nullptr);
            }
            updateLastPressedKey(nullptr);
            continue;
        }

        Acquire a(&m_cs);

        if (!m_currentKeymap) {
            injectInput(&kid, nullptr);
            Acquire b(&m_log, 0);
            m_log << "internal error: m_currentKeymap == nullptr"
                << std::endl;
            updateLastPressedKey(nullptr);
            continue;
        }

        Current c;
        c.m_keymap = m_currentKeymap;
        c.m_evdev_code = static_cast<uint16_t>(event.scanCode);

        const uint32_t MOUSE_EVENT_MARKER = 0x59414D59;
        bool isMouseEvent = (event.extraInfo == MOUSE_EVENT_MARKER);

        // IMPORTANT: Clear key object for each new event to avoid accumulation
        key = Key();
        Key mouseKey;
        Key *pProcessingKey = &key;

        if (isMouseEvent) {
            mouseKey.addScanCode(ScanCode(kid.MakeCode, kid.Flags));
            pProcessingKey = &mouseKey;
        } else {
            key.addScanCode(ScanCode(kid.MakeCode, kid.Flags));
        }

        c.m_mkey = m_setting->m_keyboard.searchKey(*pProcessingKey);
        if (c.m_mkey.m_key) {
             std::cerr << "[HANDLER:DEBUG] Key found: " << c.m_mkey.m_key->getName() << std::endl;
        } else {
             std::cerr << "[HANDLER:DEBUG] Key NOT found for scan=" << std::hex << kid.MakeCode << std::dec << std::endl;
        }

        if (!c.m_mkey.m_key) {
            if (!isMouseEvent) {
                c.m_mkey.m_key = m_setting->m_keyboard.searchPrefixKey(*pProcessingKey);
                if (c.m_mkey.m_key)
                    continue;
            }
        }

        if (c.m_mkey.m_key) {
            if (!c.m_mkey.m_key->m_isPressed && isPhysicallyPressed)
                ++ m_currentKeyPressCount;
            else if (c.m_mkey.m_key->m_isPressed && !isPhysicallyPressed)
                -- m_currentKeyPressCount;
            c.m_mkey.m_key->m_isPressed = isPhysicallyPressed;
        }

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
                injectInput(&kid, nullptr);
            }
        } else if (am == Keymap::AM_true) {
            {
                Acquire b(&m_log, 1);
                m_log << "* true modifier" << std::endl;
            }
            outputToLog(pProcessingKey, c.m_mkey, 1);
        } else if (am == Keymap::AM_oneShot || am == Keymap::AM_oneShotRepeatable) {
            {
                Acquire b(&m_log, 1);
                if (am == Keymap::AM_oneShot)
                    m_log << "* one shot modifier" << std::endl;
                else
                    m_log << "* one shot repeatable modifier" << std::endl;
            }
            outputToLog(pProcessingKey, c.m_mkey, 1);
            if (isPhysicallyPressed) {
                if (am == Keymap::AM_oneShotRepeatable &&
                        m_oneShotKey.m_key == c.m_mkey.m_key) {
                    if (m_oneShotRepeatableRepeatCount <
                            m_setting->m_oneShotRepeatableDelay) {
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
            outputToLog(pProcessingKey, c.m_mkey, 1);
            if (isPhysicallyPressed)
                m_oneShotKey.m_key = nullptr;
            beginGeneratingKeyboardEvents(c, isModifier);
        } else {
            if (kid.Flags & KEYBOARD_INPUT_DATA::E1) {
                injectInput(&kid, nullptr);
            }
        }

        if (m_currentKeyPressCount <= 0) {
            {
                Acquire b(&m_log, 1);
                m_log << "* No key is pressed" << std::endl;
            }
            generateModifierEvents(Modifier());
        }

        auto keyProcessingEnd = std::chrono::high_resolution_clock::now();
        auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            keyProcessingEnd - keyProcessingStart).count();
        yamy::metrics::PerformanceMetrics::instance().recordLatency(
            yamy::metrics::Operations::KEY_PROCESSING, static_cast<uint64_t>(durationNs));
    }
}

#endif // _WIN32
