#include "ipc_client_gui.h"

#include <algorithm>
#include <QDebug>
#include <QMetaType>

#include "core/ipc_messages.h"
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
    , m_reconnectAttempts(0)
    , m_lastConnected(false)
    , m_shouldReconnect(false)
{
    qRegisterMetaType<yamy::ipc::Message>("yamy::ipc::Message");

    if (m_channel) {
        connect(m_channel.get(),
                &yamy::platform::IIPCChannel::messageReceived,
                this,
                &IPCClientGUI::handleMessage,
                Qt::DirectConnection);
    }

    m_connectionPoller.setInterval(500);
    m_connectionPoller.setSingleShot(false);
    connect(&m_connectionPoller, &QTimer::timeout,
            this, &IPCClientGUI::pollConnectionState);

    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout,
            this, &IPCClientGUI::attemptReconnect);
}

void IPCClientGUI::connectToDaemon(const std::string& serverName)
{
    if (!serverName.empty()) {
        m_serverName = QString::fromStdString(serverName);
    }

    m_shouldReconnect = true;
    m_reconnectAttempts = 0;
    m_reconnectTimer.stop();

    if (m_channel) {
        m_channel->connect(m_serverName.toStdString());
    }
    m_connectionPoller.start();
    pollConnectionState();
}

void IPCClientGUI::disconnectFromDaemon()
{
    m_shouldReconnect = false;
    m_reconnectAttempts = 0;
    m_reconnectTimer.stop();
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
    qInfo().noquote() << "[IPCClientGUI]" << "send CmdGetStatus";
    sendMessage(yamy::MessageType::CmdGetStatus, nullptr, 0);
}

void IPCClientGUI::sendSetEnabled(bool enabled)
{
    yamy::CmdSetEnabledRequest request{};
    request.enabled = enabled;
    qInfo().noquote() << "[IPCClientGUI]" << "send CmdSetEnabled" << enabled;
    sendMessage(yamy::MessageType::CmdSetEnabled, &request, sizeof(request));
}

void IPCClientGUI::sendSwitchConfig(const QString& configName)
{
    yamy::CmdSwitchConfigRequest request{};
    copyStringField(configName, request.configName);
    qInfo().noquote() << "[IPCClientGUI]" << "send CmdSwitchConfig" << configName;
    sendMessage(yamy::MessageType::CmdSwitchConfig, &request, sizeof(request));
}

void IPCClientGUI::sendReloadConfig(const QString& configName)
{
    yamy::CmdReloadConfigRequest request{};
    copyStringField(configName, request.configName);
    qInfo().noquote() << "[IPCClientGUI]" << "send CmdReloadConfig" << configName;
    sendMessage(yamy::MessageType::CmdReloadConfig, &request, sizeof(request));
}

void IPCClientGUI::handleMessage(const yamy::ipc::Message& message)
{
    const auto rawType = static_cast<uint32_t>(message.type);
    if (rawType == static_cast<uint32_t>(yamy::MessageType::RspStatus) &&
        message.size >= sizeof(yamy::RspStatusPayload)) {
        const auto* payload = static_cast<const yamy::RspStatusPayload*>(message.data);
        qInfo().noquote() << "[IPCClientGUI]" << "received RspStatus"
                          << "engineRunning:" << payload->engineRunning
                          << "enabled:" << payload->enabled;
        emit statusReceived(*payload);
        return;
    }

    if (rawType == static_cast<uint32_t>(yamy::MessageType::RspConfigList) &&
        message.size >= sizeof(yamy::RspConfigListPayload)) {
        const auto* payload = static_cast<const yamy::RspConfigListPayload*>(message.data);
        qInfo().noquote() << "[IPCClientGUI]" << "received RspConfigList count"
                          << payload->count;
        emit configListReceived(*payload);
        return;
    }

    if (rawType == static_cast<uint32_t>(yamy::ipc::MessageType::LockStatusUpdate) &&
        message.size >= sizeof(yamy::ipc::LockStatusMessage)) {
        const auto* payload = static_cast<const yamy::ipc::LockStatusMessage*>(message.data);
        qInfo().noquote() << "[IPCClientGUI]" << "received LockStatusUpdate";
        emit lockStatusReceived(*payload);
        return;
    }
}

void IPCClientGUI::pollConnectionState()
{
    const bool connected = isConnected();
    if (connected != m_lastConnected) {
        m_lastConnected = connected;
        emit connectionStateChanged(connected);
        if (connected) {
            m_reconnectAttempts = 0;
            m_reconnectTimer.stop();
        }
    }

    if (!connected && m_shouldReconnect) {
        qInfo().noquote() << "[IPCClientGUI]" << "disconnected, scheduling reconnect";
        scheduleReconnectAttempt();
    }
}

void IPCClientGUI::attemptReconnect()
{
    if (!m_channel || !m_shouldReconnect) {
        return;
    }

    qInfo().noquote() << "[IPCClientGUI]" << "attempting reconnect to" << m_serverName;
    m_channel->connect(m_serverName.toStdString());
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

void IPCClientGUI::scheduleReconnectAttempt()
{
    static constexpr int kMaxAttempts = 3;
    static constexpr int kBackoffMs[kMaxAttempts] = {1000, 2000, 4000};

    if (m_reconnectTimer.isActive() || m_reconnectAttempts >= kMaxAttempts) {
        if (m_reconnectAttempts >= kMaxAttempts) {
            qWarning().noquote() << "[IPCClientGUI]"
                                 << "reconnect attempts exhausted";
        }
        return;
    }

    const int delay = kBackoffMs[m_reconnectAttempts];
    ++m_reconnectAttempts;
    qInfo().noquote() << "[IPCClientGUI]"
                      << "schedule reconnect attempt" << m_reconnectAttempts
                      << "in" << delay << "ms";
    m_reconnectTimer.start(delay);
}
