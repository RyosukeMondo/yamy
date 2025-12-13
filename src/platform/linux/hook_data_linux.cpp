#include "core/platform/hook_interface.h"

namespace yamy::platform {

HookData* getHookData() {
    static HookData g_hookData;
    return &g_hookData;
}

} // namespace yamy::platform
