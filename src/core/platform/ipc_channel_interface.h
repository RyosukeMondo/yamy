#pragma once

#include "core/ipc_messages.h"

#if defined(QT_CORE_LIB)
#include <QObject>

namespace yamy::platform {

class IIPCChannel : public QObject {
    Q_OBJECT
public:
    virtual ~IIPCChannel() = default;

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
};

} // namespace yamy::platform

#endif // defined(QT_CORE_LIB)
