#pragma once

#include "core/ipc_messages.h"
#include <memory>
#include <string>

#if defined(QT_CORE_LIB)
#include <QObject>

namespace yamy::platform {

class IIPCChannel : public QObject {
    Q_OBJECT
public:
    virtual ~IIPCChannel() = default;

    /// Connect to a named IPC channel
    virtual void connect(const std::string& name) = 0;

    /// Disconnect from the channel
    virtual void disconnect() = 0;

    /// Start listening for incoming connections (server mode)
    virtual void listen() = 0;

    /// Check if connected
    virtual bool isConnected() = 0;

    /// Send a message
    virtual void send(const ipc::Message& msg) = 0;

    /// Non-blocking receive (returns nullptr if no message)
    virtual std::unique_ptr<ipc::Message> nonBlockingReceive() = 0;

signals:
    void messageReceived(const yamy::ipc::Message& message);
};

} // namespace yamy::platform

#else // !defined(QT_CORE_LIB)

namespace yamy::platform {

// A non-Qt stub for headless builds. The engine just holds a pointer to this,
// so a full definition is not required for the engine itself.
class IIPCChannel {
public:
    virtual ~IIPCChannel() = default;

    /// Connect to a named IPC channel
    virtual void connect(const std::string& name) = 0;

    /// Disconnect from the channel
    virtual void disconnect() = 0;

    /// Start listening for incoming connections (server mode)
    virtual void listen() = 0;

    /// Check if connected
    virtual bool isConnected() = 0;

    /// Send a message
    virtual void send(const ipc::Message& msg) = 0;

    /// Non-blocking receive (returns nullptr if no message)
    virtual std::unique_ptr<ipc::Message> nonBlockingReceive() = 0;
};

} // namespace yamy::platform

#endif // defined(QT_CORE_LIB)
