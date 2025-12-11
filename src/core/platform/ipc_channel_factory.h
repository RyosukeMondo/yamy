#pragma once

#include "ipc_channel_interface.h"
#include "ipc_channel_null.h"
#include <memory>
#include <string>

namespace yamy::platform {

inline std::unique_ptr<IIPCChannel> createIPCChannel(const std::string& /*name*/) {
    return std::make_unique<IPCChannelNull>();
}

} // namespace yamy::platform
