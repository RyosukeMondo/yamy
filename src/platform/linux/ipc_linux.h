#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ipc_linux.h - Unix IPC using domain sockets (Track 7)

#include "../../core/platform/types.h"
#include "../../core/platform/ipc.h"

namespace yamy::platform {

class IPCLinux {
public:
    /// Send copy data to another window (process) via Unix domain socket
    static bool sendCopyData(WindowHandle sender,
                            WindowHandle target,
                            const CopyData& data,
                            uint32_t flags,
                            uint32_t timeout_ms,
                            uintptr_t* result);
};

} // namespace yamy::platform
