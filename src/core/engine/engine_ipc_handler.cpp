//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_ipc_handler.cpp - IPC message handling

#include "misc.h"
#include "engine.h"
#include "../platform/ipc.h"
#include "core/logger/journey_logger.h"
#include "core/settings/config_manager.h"

#include <algorithm>
#include <cstring>
#include <filesystem>

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

    if (rawType == static_cast<uint32_t>(yamy::MessageType::CmdGetLockStatus)) {
        if (!m_ipcChannel || !m_ipcChannel->isConnected()) {
            return;
        }

        // Send current lock state to GUI
        yamy::ipc::LockStatusMessage msg;
        const uint32_t* lockBits = m_lockState.getLockBits();
        std::memcpy(msg.lockBits, lockBits, sizeof(msg.lockBits));

        yamy::ipc::Message response;
        response.type = toIpcType(yamy::MessageType::LockStatusUpdate);
        response.data = &msg;
        response.size = sizeof(msg);
        m_ipcChannel->send(response);
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

            if (m_eventProcessor) {
                m_eventProcessor->setJourneyEventCallback(
                    [this](const yamy::logger::JourneyEvent& journey) {
                        if (m_ipcChannel && m_ipcChannel->isConnected()) {
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
