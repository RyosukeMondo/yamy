#pragma once

#include <QMainWindow>
#include "core/platform/ipc_defs.h"
#include <memory>

class QLabel;
class IPCClientGUI;
class QString;
class QPushButton;

class MainWindowGUI : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindowGUI(const QString& serverName = QString(), QWidget* parent = nullptr);
    ~MainWindowGUI() override;

private slots:
    void handleConnectionChange(bool connected);
    void handleStatusReceived(const yamy::RspStatusPayload& payload);
    void handleToggleClicked();

private:
    void updateStatusLabel(const QString& text);

    std::unique_ptr<IPCClientGUI> m_ipcClient;
    QLabel* m_connectionLabel;
    QLabel* m_statusLabel;
    QLabel* m_configLabel;
    QPushButton* m_toggleButton;
    bool m_hasStatus;
    bool m_currentEnabled;
};
