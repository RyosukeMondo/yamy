#pragma once

#include <QMainWindow>
#include <QColor>
#include <QString>
#include "core/platform/ipc_defs.h"
#include <memory>

class QLabel;
class IPCClientGUI;
class QString;
class QPushButton;
class QComboBox;

class MainWindowGUI : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindowGUI(const QString& serverName = QString(), QWidget* parent = nullptr);
    ~MainWindowGUI() override;

private slots:
    void handleConnectionChange(bool connected);
    void handleStatusReceived(const yamy::RspStatusPayload& payload);
    void handleToggleClicked();
    void handleReloadClicked();
    void handleConfigSelectionChanged(const QString& configName);

private:
    void updateStatusLabel(const QString& text);
    void updateIndicators(bool connected, bool enabled, const QString& activeConfig);
    void setIndicatorState(QLabel* indicator, const QColor& color, const QString& description);
    void createMenuBarStructure();

    std::unique_ptr<IPCClientGUI> m_ipcClient;
    QLabel* m_connectionLabel;
    QLabel* m_statusLabel;
    QLabel* m_configLabel;
    QLabel* m_connectionIndicator;
    QLabel* m_enabledIndicator;
    QLabel* m_configIndicator;
    QComboBox* m_configSelector;
    QPushButton* m_reloadButton;
    QPushButton* m_toggleButton;
    bool m_hasStatus;
    bool m_currentEnabled;
    bool m_isConnected;
    QString m_activeConfig;
};
