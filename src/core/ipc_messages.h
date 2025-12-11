#pragma once

#include "platform/types.h"
#include <cstdint>
#include <string>

namespace yamy {
namespace ipc {

// IPC message types
enum MessageType : uint32_t {
    // Command to investigate a window
    CmdInvestigateWindow = 0x1001,

    // Response with window investigation results
    RspInvestigateWindow = 0x1002,

    // Command to enable investigate mode (live logging)
    CmdEnableInvestigateMode = 0x1003,

    // Command to disable investigate mode
    CmdDisableInvestigateMode = 0x1004,

    // Notification of a key event for the live log
    NtfKeyEvent = 0x1005,

    // Control commands from yamy-ctl
    CmdReload = 0x2001,           // Reload configuration (data = config name or empty for current)
    CmdStop = 0x2002,             // Stop the engine
    CmdStart = 0x2003,            // Start the engine
    CmdGetStatus = 0x2004,        // Get engine status
    CmdGetConfig = 0x2005,        // Get configuration details
    CmdGetKeymaps = 0x2006,       // Get loaded keymaps list
    CmdGetMetrics = 0x2007,       // Get performance metrics

    // Response to control commands
    RspOk = 0x2100,               // Command succeeded (data may contain details)
    RspError = 0x2101,            // Command failed (data contains error message)
    RspStatus = 0x2102,           // Status response (data contains JSON status)
    RspConfig = 0x2103,           // Config response (data contains JSON config)
    RspKeymaps = 0x2104,          // Keymaps response (data contains JSON keymaps)
    RspMetrics = 0x2105,          // Metrics response (data contains JSON metrics)
};

// Data for CmdInvestigateWindow request
struct InvestigateWindowRequest {
    platform::WindowHandle hwnd;
};

// Data for RspInvestigateWindow response
struct InvestigateWindowResponse {
    char keymapName[256];
    char matchedClassRegex[256];
    char matchedTitleRegex[256];
    char activeModifiers[256];
    bool isDefault;
};

// Data for NtfKeyEvent notification
struct KeyEventNotification {
    char keyEvent[256];
};

struct Message {
    MessageType type;
    const void* data;
    size_t size;
};

} // namespace ipc
} // namespace yamy
