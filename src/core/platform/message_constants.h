#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// message_constants.h - Platform-agnostic message constants

#ifndef _PLATFORM_MESSAGE_CONSTANTS_H
#define _PLATFORM_MESSAGE_CONSTANTS_H

#include <cstdint>

// Undefine Windows macros that conflict with our constants
#ifdef _WIN32
    #ifdef MSGFLT_ADD
        #undef MSGFLT_ADD
    #endif
    #ifdef MSGFLT_REMOVE
        #undef MSGFLT_REMOVE
    #endif
#endif

namespace yamy::platform {
    /// Base for application-defined messages (Windows WM_APP equivalent)
    constexpr uint32_t MSG_APP_BASE = 0x8000;

    /// Application-specific message identifiers
    constexpr uint32_t MSG_APP_NOTIFY_FOCUS = MSG_APP_BASE + 103;
    constexpr uint32_t MSG_APP_NOTIFY_VKEY = MSG_APP_BASE + 104;
    constexpr uint32_t MSG_APP_ENGINE_NOTIFY = MSG_APP_BASE + 110;
    constexpr uint32_t MSG_APP_TARGET_NOTIFY = MSG_APP_BASE + 102;

    /// System message identifiers
    constexpr uint32_t MSG_COPYDATA = 0x004A;

    /// Message filter actions (values match Windows ChangeWindowMessageFilterEx)
    constexpr uint32_t MSGFLT_ADD = 1;
    constexpr uint32_t MSGFLT_REMOVE = 2;

} // namespace yamy::platform

#endif // _PLATFORM_MESSAGE_CONSTANTS_H
