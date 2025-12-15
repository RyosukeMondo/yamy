#pragma once

#ifndef _PLATFORM_IPC_DEFS_H
#define _PLATFORM_IPC_DEFS_H

#include <array>
#include <cstdint>
#include <cstring>

namespace yamy {

/// IPC Message Types for GUI Notifications
enum class MessageType : uint32_t {
    // Engine Lifecycle
    EngineStarting = 0x1001,
    EngineStarted = 0x1002,
    EngineStopping = 0x1003,
    EngineStopped = 0x1004,
    EngineError = 0x1005,

    // Configuration
    ConfigLoading = 0x2001,
    ConfigLoaded = 0x2002,
    ConfigError = 0x2003,
    ConfigValidating = 0x2004,

    // Runtime Events
    KeymapSwitched = 0x3001,
    FocusChanged = 0x3002,
    ModifierChanged = 0x3003,
    LockStatusUpdate = 0x3004,  // Lock state changed (L00-LFF)

    // Performance Metrics
    LatencyReport = 0x4001,
    CpuUsageReport = 0x4002,

    // GUI Commands (daemon control)
    CmdGetStatus = 0x5001,
    CmdSetEnabled = 0x5002,
    CmdSwitchConfig = 0x5003,
    CmdReloadConfig = 0x5004,
    CmdGetLockStatus = 0x5005,      // Request current lock state (L00-LFF)

    // GUI Responses
    RspStatus = 0x5101,
    RspConfigList = 0x5102,
};

/// Maximum lengths for string payloads
constexpr size_t kMaxConfigNameLength = 256;
constexpr size_t kMaxStatusMessageLength = 256;
constexpr size_t kMaxConfigEntries = 16;

/// Command: Request current daemon status (no payload)
struct CmdGetStatusRequest {
};

/// Command: Enable or disable the daemon from the GUI
struct CmdSetEnabledRequest {
    bool enabled{false};
};

/// Command: Switch the active configuration
struct CmdSwitchConfigRequest {
    std::array<char, kMaxConfigNameLength> configName{};

    CmdSwitchConfigRequest() {
        std::memset(configName.data(), 0, configName.size());
    }
};

/// Command: Reload a configuration (active or named)
struct CmdReloadConfigRequest {
    std::array<char, kMaxConfigNameLength> configName{};

    CmdReloadConfigRequest() {
        std::memset(configName.data(), 0, configName.size());
    }
};

/// Response: Current daemon status snapshot
struct RspStatusPayload {
    bool engineRunning{false};
    bool enabled{false};
    std::array<char, kMaxConfigNameLength> activeConfig{};
    std::array<char, kMaxStatusMessageLength> lastError{};

    RspStatusPayload() {
        std::memset(activeConfig.data(), 0, activeConfig.size());
        std::memset(lastError.data(), 0, lastError.size());
    }
};

/// Response: List of available configurations
struct RspConfigListPayload {
    uint32_t count{0};
    std::array<std::array<char, kMaxConfigNameLength>, kMaxConfigEntries> configs{};

    RspConfigListPayload() {
        for (auto& entry : configs) {
            std::memset(entry.data(), 0, entry.size());
        }
    }
};

} // namespace yamy

#endif // _PLATFORM_IPC_DEFS_H
