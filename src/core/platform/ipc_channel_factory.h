#pragma once

#include "ipc_channel_interface.h"
#include "ipc_channel_null.h"

#if defined(__linux__) && defined(QT_CORE_LIB)
#include "linux/ipc_channel_qt.h"
#endif

#include <memory>
#include <string>

namespace yamy::platform {

inline std::unique_ptr<IIPCChannel> createIPCChannel(const std::string& name) {
#if defined(__linux__) && defined(QT_CORE_LIB)
    return std::make_unique<IPCChannelQt>(name);
#else
    (void)name;  // Suppress unused parameter warning
    return std::make_unique<IPCChannelNull>();
#endif
}

} // namespace yamy::platform
