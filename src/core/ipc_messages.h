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
