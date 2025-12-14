#include "ipc_client_gui.h"

#include <algorithm>

#include "core/platform/ipc_channel_factory.h"

namespace {
yamy::ipc::MessageType toWireType(yamy::MessageType type) {
    return static_cast<yamy::ipc::MessageType>(static_cast<uint32_t>(type));
}
}  // namespace

IPCClientGUI::IPCClientGUI(QObject* parent)
    : QObject(parent)
    , m_channel(yamy::platform::createIPCChannel("yamy-gui"))
    , m_serverName(QStringLiteral("yamy-engine"))
    , m_lastConnected(false)
{
    if (m_channel) {
        connect(m_channel.get(),
                &yamy::platform::IIPCChannel::messageReceived,
                this,
                &IPCClientGUI::handleMessage,
                Qt::QueuedConnection);
    }

    m_connectionPoller.setInterval(500);
    m_connectionPoller.setSingleShot(false);
    connect(&m_connectionPoller, &QTimer::timeout,
            this, &IPCClientGUI::pollConnectionState);
}

void IPCClientGUI::connectToDaemon(const std::string& serverName)
{
    if (!serverName.empty()) {
        m_serverName = QString::fromStdString(serverName);
    }

    if (m_channel) {
        m_channel->connect(m_serverName.toStdString());
    }
    m_connectionPoller.start();
    pollConnectionState();
}

void IPCClientGUI::disconnectFromDaemon()
{
    m_connectionPoller.stop();
    if (m_channel) {
        m_channel->disconnect();
    }
    pollConnectionState();
}

bool IPCClientGUI::isConnected() const
{
    return m_channel && m_channel->isConnected();
}

void IPCClientGUI::sendGetStatus()
{
    sendMessage(yamy::MessageType::CmdGetStatus, nullptr, 0);
}

void IPCClientGUI::sendSetEnabled(bool enabled)
{
    yamy::CmdSetEnabledRequest request{};
    request.enabled = enabled;
    sendMessage(yamy::MessageType::CmdSetEnabled, &request, sizeof(request));
}

void IPCClientGUI::sendSwitchConfig(const QString& configName)
{
    yamy::CmdSwitchConfigRequest request{};
    copyStringField(configName, request.configName);
    sendMessage(yamy::MessageType::CmdSwitchConfig, &request, sizeof(request));
}

void IPCClientGUI::sendReloadConfig(const QString& configName)
{
    yamy::CmdReloadConfigRequest request{};
    copyStringField(configName, request.configName);
    sendMessage(yamy::MessageType::CmdReloadConfig, &request, sizeof(request));
}

void IPCClientGUI::handleMessage(const yamy::ipc::Message& message)
{
    const auto rawType = static_cast<uint32_t>(message.type);
    if (rawType == static_cast<uint32_t>(yamy::MessageType::RspStatus) &&
        message.size >= sizeof(yamy::RspStatusPayload)) {
        const auto* payload = static_cast<const yamy::RspStatusPayload*>(message.data);
        emit statusReceived(*payload);
        return;
    }

    if (rawType == static_cast<uint32_t>(yamy::MessageType::RspConfigList) &&
        message.size >= sizeof(yamy::RspConfigListPayload)) {
        const auto* payload = static_cast<const yamy::RspConfigListPayload*>(message.data);
        emit configListReceived(*payload);
        return;
    }
}

void IPCClientGUI::pollConnectionState()
{
    const bool connected = isConnected();
    if (connected != m_lastConnected) {
        m_lastConnected = connected;
        emit connectionStateChanged(connected);
    }
}

template <size_t N>
void IPCClientGUI::copyStringField(const QString& value, std::array<char, N>& buffer)
{
    std::fill(buffer.begin(), buffer.end(), '\0');
    const auto utf8 = value.toUtf8();
    const auto copyLen = std::min(static_cast<size_t>(utf8.size()), N - 1);
    std::copy_n(utf8.constData(), copyLen, buffer.begin());
}

void IPCClientGUI::sendMessage(yamy::MessageType type, const void* data, size_t size)
{
    if (!m_channel) {
        return;
    }

    yamy::ipc::Message msg{};
    msg.type = toWireType(type);
    msg.data = data;
    msg.size = size;
    m_channel->send(msg);
}
