#pragma once

#include "ipc_channel_interface.h"

namespace yamy::platform {

class IPCChannelNull : public IIPCChannel {
public:
    void connect(const std::string&) override {}
    void disconnect() override {}
    void listen() override {}
    bool isConnected() override { return false; }
    void send(const ipc::Message&) override {}
    std::unique_ptr<ipc::Message> nonBlockingReceive() override { return nullptr; }
};

} // namespace yamy::platform
