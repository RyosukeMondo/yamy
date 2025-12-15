#pragma once

#include <QMainWindow>
#include <QColor>
#include <QString>
#include "core/platform/ipc_defs.h"
#include <QPointer>
#include <memory>

class QLabel;
class IPCClientGUI;
class QString;
class QPushButton;
class QComboBox;
class DialogLogQt;
class DialogInvestigateQt;
class DialogSettingsQt;
class PreferencesDialog;
class DialogAboutQt;
class DialogShortcutsQt;
class DialogExamplesQt;
class ConfigManagerDialog;
class LockIndicatorWidget;
namespace yamy {
namespace ui {
class NotificationHistoryDialog;
}
}
class Engine;

class MainWindowGUI : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindowGUI(const QString& serverName = QString(), Engine* engine = nullptr, QWidget* parent = nullptr);
    ~MainWindowGUI() override;

private slots:
    void handleConnectionChange(bool connected);
    void handleStatusReceived(const yamy::RspStatusPayload& payload);
    void handleConfigListReceived(const yamy::RspConfigListPayload& payload);
    void handleToggleClicked();
    void handleReloadClicked();
    void handleConfigSelectionChanged(const QString& configName);
    void showLogDialog();
    void showInvestigateDialog();
    void showSettingsDialog();
    void showPreferencesDialog();
    void showAboutDialog();
    void showKeyboardShortcutsDialog();
    void showExamplesDialog();
    void showConfigManagerDialog();
    void showNotificationHistoryDialog();

private:
    void updateStatusLabel(const QString& text);
    void updateIndicators(bool connected, bool enabled, const QString& activeConfig);
    void setIndicatorState(QLabel* indicator, const QColor& color, const QString& description);
    void createMenuBarStructure();

    std::unique_ptr<IPCClientGUI> m_ipcClient;
    Engine* m_engine;
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
    QString m_lastError;
    bool m_updatingConfigList{false};
    QPointer<DialogLogQt> m_logDialog;
    QPointer<DialogInvestigateQt> m_investigateDialog;
    QPointer<DialogSettingsQt> m_settingsDialog;
    QPointer<PreferencesDialog> m_preferencesDialog;
    QPointer<DialogAboutQt> m_aboutDialog;
    QPointer<DialogShortcutsQt> m_shortcutsDialog;
    QPointer<DialogExamplesQt> m_examplesDialog;
    QPointer<ConfigManagerDialog> m_configManagerDialog;
    QPointer<yamy::ui::NotificationHistoryDialog> m_notificationHistoryDialog;
    LockIndicatorWidget* m_lockIndicatorWidget;
};
