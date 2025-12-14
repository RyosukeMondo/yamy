#pragma once

#include <QMainWindow>
#include "core/platform/ipc_defs.h"
#include <memory>

class QLabel;
class IPCClientGUI;

class MainWindowGUI : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindowGUI(QWidget* parent = nullptr);
    ~MainWindowGUI() override;

private slots:
    void handleConnectionChange(bool connected);
    void handleStatusReceived(const yamy::RspStatusPayload& payload);

private:
    void updateStatusLabel(const QString& text);

    std::unique_ptr<IPCClientGUI> m_ipcClient;
    QLabel* m_connectionLabel;
};
