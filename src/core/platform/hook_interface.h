#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// hook_interface.h - Platform-agnostic hook data interface

#ifndef _PLATFORM_HOOK_INTERFACE_H
#define _PLATFORM_HOOK_INTERFACE_H

#include <cstdint>

namespace yamy::platform {
    /// Mouse position structure
    struct MousePosition {
        int32_t x;
        int32_t y;
    };

    /// Mouse hook type enumeration
    enum class MouseHookType : uint32_t {
        None = 0,
        Wheel = 1 << 0,
        WindowMove = 1 << 1,
    };

    /// Hook data interface
    struct HookData {
        uint16_t m_syncKey;                /// Sync key scan code
        bool m_syncKeyIsExtended;          /// Sync key extended flag
        bool m_doesNotifyCommand;          /// Command notification flag
        uint32_t m_hwndTaskTray;           /// Task tray window handle
        bool m_correctKanaLockHandling;    /// KanaLock handling flag
        MouseHookType m_mouseHookType;     /// Mouse hook type
        int32_t m_mouseHookParam;          /// Mouse hook parameter
        uint32_t m_hwndMouseHookTarget;    /// Target window for mouse hook
        MousePosition m_mousePos;          /// Current mouse position
    };

    /// Get the global hook data instance
    /// @return Pointer to hook data (never null)
    HookData* getHookData();

} // namespace yamy::platform

#endif // _PLATFORM_HOOK_INTERFACE_H
