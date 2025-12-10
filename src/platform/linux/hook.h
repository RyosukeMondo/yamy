#pragma once
#ifndef _HOOK_H
#define _HOOK_H

namespace yamy {
namespace platform {
    // Stub for Linux
}
}

///
enum MouseHookType {
    MouseHookType_None = 0,                /// none
    MouseHookType_Wheel = 1 << 0,            /// wheel
    MouseHookType_WindowMove = 1 << 1,        /// window move
};

#endif // _HOOK_H
