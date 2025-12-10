#pragma once

#ifndef _PLATFORM_IPC_DEFS_H
#define _PLATFORM_IPC_DEFS_H

#include <cstdint>

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

    // Performance Metrics
    LatencyReport = 0x4001,
    CpuUsageReport = 0x4002,
};

} // namespace yamy

#endif // _PLATFORM_IPC_DEFS_H
