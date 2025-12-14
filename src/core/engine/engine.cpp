//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "../platform/sync.h"
#include "../platform/ipc.h"
#include "core/logging/logger.h"
#include "core/logger/journey_logger.h"
#include "../../utils/metrics.h"

#include <iomanip>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <type_traits>

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

        checkFocusWindow();

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

        if (!m_currentFocusOfThread ||
                !m_currentKeymap) {
            injectInput(&kid, nullptr);
            Acquire b(&m_log, 0);
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
        bool isPhysicallyPressed = event.isKeyDown;

        checkFocusWindow();

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

        if (!m_currentFocusOfThread ||
                !m_currentKeymap) {
            injectInput(&kid, nullptr);
            Acquire b(&m_log, 0);
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
        }

        auto keyProcessingEnd = std::chrono::high_resolution_clock::now();
        auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            keyProcessingEnd - keyProcessingStart).count();
        yamy::metrics::PerformanceMetrics::instance().recordLatency(
            yamy::metrics::Operations::KEY_PROCESSING, static_cast<uint64_t>(durationNs));
    }
}

#endif // _WIN32

void Engine::handleIpcMessage(const yamy::ipc::Message& message)
{
    const auto toIpcType = [](yamy::MessageType type) {
        return static_cast<yamy::ipc::MessageType>(static_cast<uint32_t>(type));
    };

    auto copyStringField = [](const std::string& value, auto& buffer) {
        using BufferType = std::decay_t<decltype(buffer)>;
        std::fill(buffer.begin(), buffer.end(), '\0');
        const auto copyLen = std::min(value.size(), buffer.size() - 1);
        std::memcpy(buffer.data(), value.data(), copyLen);
    };

    auto sendGuiStatus = [&](const std::string& lastErrorMessage) {
        if (!m_ipcChannel || !m_ipcChannel->isConnected()) {
            return;
        }

        yamy::RspStatusPayload payload;
        payload.engineRunning = (getState() == yamy::EngineState::Running);
        payload.enabled = getIsEnabled();
        copyStringField(ConfigManager::instance().getActiveConfig(), payload.activeConfig);
        copyStringField(lastErrorMessage, payload.lastError);

        yamy::ipc::Message response;
        response.type = toIpcType(yamy::MessageType::RspStatus);
        response.data = &payload;
        response.size = sizeof(payload);
        m_ipcChannel->send(response);
    };

    auto sendGuiConfigList = [&]() {
        if (!m_ipcChannel || !m_ipcChannel->isConnected()) {
            return;
        }

        yamy::RspConfigListPayload payload;
        auto configs = ConfigManager::instance().listConfigs();
        payload.count = std::min<uint32_t>(configs.size(), yamy::kMaxConfigEntries);

        for (uint32_t i = 0; i < payload.count; ++i) {
            const auto& entry = configs[i];
            std::string displayName = entry.name.empty()
                ? std::filesystem::path(entry.path).stem().string()
                : entry.name;
            copyStringField(displayName, payload.configs[i]);
        }

        yamy::ipc::Message response;
        response.type = toIpcType(yamy::MessageType::RspConfigList);
        response.data = &payload;
        response.size = sizeof(payload);
        m_ipcChannel->send(response);
    };

    const uint32_t rawType = static_cast<uint32_t>(message.type);
    if (rawType == static_cast<uint32_t>(yamy::MessageType::CmdGetStatus)) {
        sendGuiStatus("");
        sendGuiConfigList();
        return;
    }

    if (rawType == static_cast<uint32_t>(yamy::MessageType::CmdSetEnabled)) {
        std::string error;

        if (message.size >= sizeof(yamy::CmdSetEnabledRequest) && message.data) {
            const auto* request = static_cast<const yamy::CmdSetEnabledRequest*>(message.data);
            enable(request->enabled);
        } else {
            error = "Invalid CmdSetEnabled payload";
        }

        sendGuiStatus(error);
        sendGuiConfigList();
        return;
    }

    if (rawType == static_cast<uint32_t>(yamy::MessageType::CmdSwitchConfig)) {
        std::string error;
        std::string requestedName;

        if (message.size >= sizeof(yamy::CmdSwitchConfigRequest) && message.data) {
            const auto* request = static_cast<const yamy::CmdSwitchConfigRequest*>(message.data);
            requestedName = std::string(request->configName.data());
        } else {
            error = "Invalid CmdSwitchConfig payload";
        }

        if (error.empty()) {
            auto configs = ConfigManager::instance().listConfigs();
            auto it = std::find_if(configs.begin(), configs.end(), [&](const ConfigEntry& entry) {
                return entry.name == requestedName || entry.path == requestedName;
            });

            if (it == configs.end()) {
                error = "Config not found: " + requestedName;
            } else {
                const std::string targetPath = it->path;
                if (switchConfiguration(targetPath)) {
                    ConfigManager::instance().setActiveConfig(targetPath);
                } else {
                    error = "Failed to switch config: " + targetPath;
                }
            }
        }

        sendGuiStatus(error);
        sendGuiConfigList();
        return;
    }

    if (rawType == static_cast<uint32_t>(yamy::MessageType::CmdReloadConfig)) {
        std::string error;
        std::string requestedName;

        if (message.size >= sizeof(yamy::CmdReloadConfigRequest) && message.data) {
            const auto* request = static_cast<const yamy::CmdReloadConfigRequest*>(message.data);
            requestedName = std::string(request->configName.data());
        } else {
            error = "Invalid CmdReloadConfig payload";
        }

        auto configs = ConfigManager::instance().listConfigs();
        std::string targetPath;

        if (error.empty()) {
            if (!requestedName.empty()) {
                auto it = std::find_if(configs.begin(), configs.end(), [&](const ConfigEntry& entry) {
                    return entry.name == requestedName || entry.path == requestedName;
                });
                if (it != configs.end()) {
                    targetPath = it->path;
                } else {
                    error = "Config not found: " + requestedName;
                }
            } else {
                targetPath = ConfigManager::instance().getActiveConfig();
                if (targetPath.empty()) {
                    error = "No active config to reload";
                }
            }
        }

        if (error.empty()) {
            if (switchConfiguration(targetPath)) {
                ConfigManager::instance().setActiveConfig(targetPath);
            } else {
                error = "Failed to reload config: " + targetPath;
            }
        }

        sendGuiStatus(error);
        sendGuiConfigList();
        return;
    }

    switch (message.type) {
        case yamy::ipc::CmdEnableInvestigateMode:
            m_isInvestigateMode = true;

            // Register journey event callback to send formatted events to Investigate Window
            if (m_eventProcessor) {
                m_eventProcessor->setJourneyEventCallback(
                    [this](const yamy::logger::JourneyEvent& journey) {
                        if (m_ipcChannel && m_ipcChannel->isConnected()) {
                            // Format the journey event using the same formatter as console output
                            std::string formattedLine = yamy::logger::JourneyLogger::formatJourneyLine(journey);

                            yamy::ipc::KeyEventNotification notification;
                            strncpy(notification.keyEvent, formattedLine.c_str(), sizeof(notification.keyEvent) - 1);
                            notification.keyEvent[sizeof(notification.keyEvent) - 1] = '\0';

                            yamy::ipc::Message msg;
                            msg.type = yamy::ipc::NtfKeyEvent;
                            msg.data = &notification;
                            msg.size = sizeof(notification);

                            m_ipcChannel->send(msg);
                        }
                    }
                );
            }
            break;

        case yamy::ipc::CmdDisableInvestigateMode:
            m_isInvestigateMode = false;

            // Clear journey event callback
            if (m_eventProcessor) {
                m_eventProcessor->setJourneyEventCallback(nullptr);
            }
            break;
        case yamy::ipc::CmdInvestigateWindow:
        {
            if (message.size >= sizeof(yamy::ipc::InvestigateWindowRequest)) {
                const auto* request = static_cast<const yamy::ipc::InvestigateWindowRequest*>(message.data);
                
                std::string className = m_windowSystem->getClassName(request->hwnd);
                std::string titleName = m_windowSystem->getWindowText(request->hwnd);

                KeymapStatus status = queryKeymapForWindow(request->hwnd, className, titleName);

                yamy::ipc::InvestigateWindowResponse response;
                strncpy(response.keymapName, status.keymapName.c_str(), sizeof(response.keymapName) - 1);
                strncpy(response.matchedClassRegex, status.matchedClassRegex.c_str(), sizeof(response.matchedClassRegex) - 1);
                strncpy(response.matchedTitleRegex, status.matchedTitleRegex.c_str(), sizeof(response.matchedTitleRegex) - 1);
                strncpy(response.activeModifiers, status.activeModifiers.c_str(), sizeof(response.activeModifiers) - 1);
                response.isDefault = status.isDefault;

                yamy::ipc::Message responseMessage;
                responseMessage.type = yamy::ipc::RspInvestigateWindow;
                responseMessage.data = &response;
                responseMessage.size = sizeof(response);

                // Send response back to dialog
                if (m_ipcChannel && m_ipcChannel->isConnected()) {
                    m_ipcChannel->send(responseMessage);
                }
            }
            break;
        }
        default:
            break;
    }
}
