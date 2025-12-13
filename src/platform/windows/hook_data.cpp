//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// hook_data.cpp - Windows hook data implementation

#ifdef _WIN32

#include "../../core/platform/hook_interface.h"
#include "hook.h"  // For g_hookData and Windows HookData

namespace yamy::platform {

// Direct accessor to Windows g_hookData with type casting
// The Windows HookData and platform HookData are binary-compatible
// due to careful struct layout matching
HookData* getHookData() {
    // Directly cast g_hookData to platform HookData*
    // This works because the struct layouts are compatible:
    // - Both start with m_syncKey (uint16_t / USHORT)
    // - Followed by bools in same order
    // - DWORD and uint32_t are same size
    // - MouseHookType enum is same size
    // - POINT and MousePosition have same layout
    static_assert(sizeof(::HookData) == sizeof(HookData),
                  "Windows HookData and platform HookData must be same size");

    return reinterpret_cast<HookData*>(g_hookData);
}

} // namespace yamy::platform

#endif // _WIN32
