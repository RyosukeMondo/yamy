#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <array>
#include <memory>
#include <string>

#include "core/platform/ipc_defs.h"
#include "core/platform/ipc_channel_interface.h"

/**
 * @brief High-level GUI IPC client that wraps IPCChannelQt with GUI-specific commands.
 *
 * Provides asynchronous helpers for sending GUI control commands to the daemon and
 * emits Qt signals for structured responses and connection state changes.
 */
class IPCClientGUI : public QObject {
    Q_OBJECT
public:
    explicit IPCClientGUI(QObject* parent = nullptr);

    /// Connect to the daemon IPC server (default: "yamy-engine")
    void connectToDaemon(const std::string& serverName = "yamy-engine");

    /// Disconnect from the daemon IPC server
    void disconnectFromDaemon();

    /// Whether the underlying IPC channel reports as connected
    bool isConnected() const;

    /// Request current status and config list
    void sendGetStatus();

    /// Request current lock state (L00-LFF)
    void sendGetLockStatus();

    /// Toggle enabled/disabled state
    void sendSetEnabled(bool enabled);

    /// Switch to a specific configuration name/path
    void sendSwitchConfig(const QString& configName);

    /// Reload the active or named configuration
    void sendReloadConfig(const QString& configName);

    /// Add a new configuration file to the list
    void sendAddConfig(const QString& configPath);

    /// Remove a configuration file from the list
    void sendRemoveConfig(const QString& configPath);

signals:
    void statusReceived(const yamy::RspStatusPayload& payload);
    void configListReceived(const yamy::RspConfigListPayload& payload);
    void connectionStateChanged(bool connected);
    void lockStatusReceived(const yamy::ipc::LockStatusMessage& lockStatus);

private slots:
    void handleMessage(const yamy::ipc::Message& message);
    void pollConnectionState();
    void attemptReconnect();

private:
    template <size_t N>
    void copyStringField(const QString& value, std::array<char, N>& buffer);
    void sendMessage(yamy::MessageType type, const void* data, size_t size);
    void scheduleReconnectAttempt();

    std::unique_ptr<yamy::platform::IIPCChannel> m_channel;
    QString m_serverName;
    QTimer m_connectionPoller;
    QTimer m_reconnectTimer;
    int m_reconnectAttempts;
    bool m_lastConnected;
    bool m_shouldReconnect;
};
