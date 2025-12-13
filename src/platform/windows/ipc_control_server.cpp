#include "ipc_control_server.h"

namespace yamy {
namespace platform {

IPCControlServer::IPCControlServer(const std::string& socketPath)
    : m_socketPath(socketPath), m_running(false)
{
}

IPCControlServer::~IPCControlServer()
{
}

void IPCControlServer::setCommandCallback(ControlCommandCallback callback)
{
    m_callback = callback;
}

bool IPCControlServer::start()
{
    // Windows implementation not yet available
    m_running = true;
    return true;
}

void IPCControlServer::stop()
{
    m_running = false;
}

bool IPCControlServer::isRunning() const
{
    return m_running;
}

} // namespace platform
} // namespace yamy
