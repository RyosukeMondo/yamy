#include "main_window_gui.h"

#include <QAction>
#include <QComboBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#include "config_manager_dialog.h"
#include "dialog_about_qt.h"
#include "dialog_examples_qt.h"
#include "dialog_investigate_qt.h"
#include "dialog_log_qt.h"
#include "dialog_settings_qt.h"
#include "dialog_shortcuts_qt.h"
#include "ipc_client_gui.h"
#include "lock_indicator_widget.h"
#include "notification_history.h"
#include "preferences_dialog.h"
#include "core/ipc_messages.h"

MainWindowGUI::MainWindowGUI(const QString& serverName, Engine* engine, QWidget* parent)
    : QMainWindow(parent),
      m_ipcClient(std::make_unique<IPCClientGUI>(this)),
      m_engine(engine),
      m_connectionLabel(new QLabel(this)),
      m_statusLabel(new QLabel(this)),
      m_configLabel(new QLabel(this)),
      m_connectionIndicator(new QLabel(this)),
      m_enabledIndicator(new QLabel(this)),
      m_configIndicator(new QLabel(this)),
      m_configSelector(new QComboBox(this)),
      m_reloadButton(new QPushButton(this)),
      m_toggleButton(new QPushButton(this)),
      m_hasStatus(false),
      m_currentEnabled(false),
      m_isConnected(false),
      m_lockIndicatorWidget(new LockIndicatorWidget(this)) {
    setWindowTitle(tr("YAMY GUI"));
    createMenuBarStructure();

    auto* centralWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    const auto setupIndicator = [](QLabel* indicator, const QString& name) {
        indicator->setFixedSize(14, 14);
        indicator->setStyleSheet(QStringLiteral("background-color: #888; border: 1px solid #444; border-radius: 7px;"));
        indicator->setAccessibleName(name);
        indicator->setToolTip(name);
    };
    setupIndicator(m_connectionIndicator, tr("Connection state"));
    setupIndicator(m_enabledIndicator, tr("Engine enabled state"));
    setupIndicator(m_configIndicator, tr("Config state"));

    m_connectionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_configLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_toggleButton->setEnabled(false);
    m_reloadButton->setEnabled(false);
    m_configSelector->setEnabled(false);

    auto* connectionRow = new QHBoxLayout();
    connectionRow->setSpacing(8);
    connectionRow->addWidget(m_connectionIndicator);
    connectionRow->addWidget(m_connectionLabel, /*stretch*/ 1);
    layout->addLayout(connectionRow);

    auto* statusRow = new QHBoxLayout();
    statusRow->setSpacing(8);
    statusRow->addWidget(m_enabledIndicator);
    statusRow->addWidget(m_statusLabel, /*stretch*/ 1);
    statusRow->addWidget(m_toggleButton, /*stretch*/ 0);
    layout->addLayout(statusRow);

    auto* configRow = new QHBoxLayout();
    configRow->setSpacing(8);
    configRow->addWidget(m_configIndicator);
    configRow->addWidget(m_configLabel, /*stretch*/ 1);
    m_configSelector->setEditable(false);
    m_configSelector->setPlaceholderText(tr("Select a configuration"));
    configRow->addWidget(m_configSelector, /*stretch*/ 1);
    m_reloadButton->setText(tr("Reload"));
    configRow->addWidget(m_reloadButton, /*stretch*/ 0);
    layout->addLayout(configRow);

    // Add lock indicator widget
    auto* lockGroup = new QWidget(this);
    auto* lockLayout = new QVBoxLayout(lockGroup);
    lockLayout->setContentsMargins(0, 8, 0, 0);
    lockLayout->setSpacing(4);
    auto* lockTitle = new QLabel(tr("Lock Status:"), lockGroup);
    lockTitle->setStyleSheet("font-weight: bold;");
    lockLayout->addWidget(lockTitle);
    lockLayout->addWidget(m_lockIndicatorWidget);
    layout->addWidget(lockGroup);

    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    updateStatusLabel(tr("Connecting to daemon..."));
    m_statusLabel->setText(tr("Status: unknown"));
    m_configLabel->setText(tr("Active config: -"));
    m_toggleButton->setText(tr("Enable"));
    updateIndicators(false, false, QString());

    connect(m_ipcClient.get(), &IPCClientGUI::connectionStateChanged, this, &MainWindowGUI::handleConnectionChange);
    connect(m_ipcClient.get(), &IPCClientGUI::statusReceived, this, &MainWindowGUI::handleStatusReceived);
    connect(m_ipcClient.get(), &IPCClientGUI::configListReceived, this, &MainWindowGUI::handleConfigListReceived);
    connect(m_ipcClient.get(), &IPCClientGUI::lockStatusReceived, this, [this](const yamy::ipc::LockStatusMessage& lockStatus) {
        if (m_lockIndicatorWidget) {
            m_lockIndicatorWidget->updateLockStatus(lockStatus.lockBits);
        }
    });
    connect(m_toggleButton, &QPushButton::clicked, this, &MainWindowGUI::handleToggleClicked);
    connect(m_reloadButton, &QPushButton::clicked, this, &MainWindowGUI::handleReloadClicked);
    connect(m_configSelector, &QComboBox::currentTextChanged, this, &MainWindowGUI::handleConfigSelectionChanged);

    if (!serverName.isEmpty()) {
        m_ipcClient->connectToDaemon(serverName.toStdString());
    } else {
        m_ipcClient->connectToDaemon();
    }
}

MainWindowGUI::~MainWindowGUI() {
    if (m_ipcClient) {
        m_ipcClient->disconnectFromDaemon();
    }
}

void MainWindowGUI::handleConnectionChange(bool connected) {
    m_isConnected = connected;
    updateStatusLabel(connected ? tr("Connected to daemon") : tr("Disconnected from daemon"));
    m_toggleButton->setEnabled(connected && m_hasStatus);
    m_reloadButton->setEnabled(false);
    m_configSelector->setEnabled(false);
    m_lastError.clear();
    if (!connected) {
        m_hasStatus = false;
        m_activeConfig.clear();
        m_updatingConfigList = true;
        m_configSelector->clear();
        m_updatingConfigList = false;
        m_statusLabel->setText(tr("Status: unknown"));
        m_configLabel->setText(tr("Active config: -"));
        m_toggleButton->setText(tr("Enable"));
    }
    updateIndicators(m_isConnected, m_currentEnabled, m_activeConfig);
    if (connected && m_ipcClient) {
        m_ipcClient->sendGetStatus();
    }
}

void MainWindowGUI::handleStatusReceived(const yamy::RspStatusPayload& payload) {
    const QString enabledText = payload.enabled ? tr("Enabled") : tr("Disabled");
    const QString configText = QString::fromLatin1(payload.activeConfig.data());
    const QString lastErrorText = QString::fromLatin1(payload.lastError.data()).trimmed();

    m_hasStatus = true;
    m_currentEnabled = payload.enabled;
    m_activeConfig = configText;
    if (!lastErrorText.isEmpty() && lastErrorText != m_lastError) {
        QMessageBox::warning(this, tr("Daemon error"), lastErrorText);
    }
    m_lastError = lastErrorText;

    updateStatusLabel(tr("Connected (%1)").arg(enabledText));
    const QString statusDetails = lastErrorText.isEmpty()
        ? enabledText
        : tr("%1 (last error: %2)").arg(enabledText, lastErrorText);
    m_statusLabel->setText(tr("Status: %1").arg(statusDetails));
    m_configLabel->setText(configText.isEmpty() ? tr("Active config: (none)") : tr("Active config: %1").arg(configText));
    m_toggleButton->setText(payload.enabled ? tr("Disable") : tr("Enable"));
    m_toggleButton->setEnabled(m_ipcClient && m_ipcClient->isConnected());
    m_reloadButton->setText(tr("Reload"));
    const bool canUseConfigs = m_ipcClient && m_ipcClient->isConnected() && m_configSelector->count() > 0;
    m_configSelector->setEnabled(canUseConfigs);
    m_reloadButton->setEnabled(canUseConfigs);
    updateIndicators(m_isConnected, m_currentEnabled, m_activeConfig);
}

void MainWindowGUI::handleConfigListReceived(const yamy::RspConfigListPayload& payload) {
    QStringList configs;
    configs.reserve(static_cast<int>(payload.count));
    for (uint32_t i = 0; i < payload.count; ++i) {
        const QString entry = QString::fromLatin1(payload.configs[i].data()).trimmed();
        if (!entry.isEmpty()) {
            configs.push_back(entry);
        }
    }

    m_updatingConfigList = true;
    m_configSelector->clear();
    if (!configs.isEmpty()) {
        m_configSelector->addItems(configs);
        m_configSelector->setPlaceholderText(tr("Select a configuration"));
        if (!m_activeConfig.isEmpty()) {
            const int index = m_configSelector->findText(m_activeConfig);
            if (index >= 0) {
                m_configSelector->setCurrentIndex(index);
            }
        }
    } else {
        m_configSelector->setPlaceholderText(tr("No configurations available"));
    }
    m_updatingConfigList = false;

    const bool hasConfigs = !configs.isEmpty() && m_ipcClient && m_ipcClient->isConnected();
    m_configSelector->setEnabled(hasConfigs);
    m_reloadButton->setEnabled(hasConfigs);
    updateIndicators(m_isConnected, m_currentEnabled, m_activeConfig);
}

void MainWindowGUI::handleToggleClicked() {
    if (!m_ipcClient || !m_hasStatus || !m_ipcClient->isConnected()) {
        return;
    }
    const bool targetEnabled = !m_currentEnabled;
    m_toggleButton->setEnabled(false);
    m_toggleButton->setText(targetEnabled ? tr("Enabling...") : tr("Disabling..."));
    m_ipcClient->sendSetEnabled(targetEnabled);
}

void MainWindowGUI::handleReloadClicked() {
    if (!m_ipcClient || !m_hasStatus || !m_ipcClient->isConnected()) {
        return;
    }

    const QString targetConfig = m_configSelector->currentText().trimmed().isEmpty()
        ? m_activeConfig
        : m_configSelector->currentText().trimmed();

    m_reloadButton->setEnabled(false);
    m_configSelector->setEnabled(false);
    m_toggleButton->setEnabled(false);
    m_reloadButton->setText(tr("Reloading..."));
    m_statusLabel->setText(targetConfig.isEmpty()
        ? tr("Status: reloading active config...")
        : tr("Status: reloading %1...").arg(targetConfig));

    qInfo().noquote() << "[MainWindowGUI]" << "Reload requested for" << (targetConfig.isEmpty() ? QStringLiteral("<active>") : targetConfig);
    m_ipcClient->sendReloadConfig(targetConfig);
}

void MainWindowGUI::handleConfigSelectionChanged(const QString& configName) {
    if (m_updatingConfigList || !m_ipcClient || !m_hasStatus || !m_ipcClient->isConnected()) {
        return;
    }

    const QString trimmedName = configName.trimmed();
    if (trimmedName.isEmpty() || trimmedName == m_activeConfig) {
        return;
    }

    m_configSelector->setEnabled(false);
    m_reloadButton->setEnabled(false);
    m_toggleButton->setEnabled(false);
    m_statusLabel->setText(tr("Status: switching to %1...").arg(trimmedName));
    setIndicatorState(m_configIndicator, QColor("#f1c40f"), tr("Switching to %1").arg(trimmedName));

    qInfo().noquote() << "[MainWindowGUI]" << "Switch config requested:" << trimmedName;
    m_ipcClient->sendSwitchConfig(trimmedName);
}

void MainWindowGUI::updateStatusLabel(const QString& text) {
    if (m_connectionLabel) {
        m_connectionLabel->setText(text);
    }
    qInfo().noquote() << "[MainWindowGUI]" << text;
}

void MainWindowGUI::updateIndicators(bool connected, bool enabled, const QString& activeConfig) {
    const QColor connectionColor = connected ? QColor("#2ecc71") : QColor("#e74c3c");
    setIndicatorState(m_connectionIndicator, connectionColor, connected ? tr("Connected to daemon") : tr("Disconnected from daemon"));

    if (!m_hasStatus) {
        setIndicatorState(m_enabledIndicator, QColor("#f1c40f"), tr("Status unknown"));
        setIndicatorState(m_configIndicator, QColor("#f1c40f"), tr("Config unknown"));
        return;
    }

    setIndicatorState(m_enabledIndicator,
                      enabled ? QColor("#2ecc71") : QColor("#e67e22"),
                      enabled ? tr("Daemon enabled") : tr("Daemon disabled"));

    const bool hasConfig = !activeConfig.isEmpty();
    setIndicatorState(m_configIndicator,
                      hasConfig ? QColor("#3498db") : QColor("#f1c40f"),
                      hasConfig ? tr("Active config: %1").arg(activeConfig) : tr("No active config"));
}

void MainWindowGUI::setIndicatorState(QLabel* indicator, const QColor& color, const QString& description) {
    if (!indicator) {
        return;
    }
    indicator->setStyleSheet(QStringLiteral("background-color: %1; border: 1px solid #444; border-radius: 7px;").arg(color.name()));
    indicator->setToolTip(description);
    indicator->setAccessibleDescription(description);
}

void MainWindowGUI::createMenuBarStructure() {
    auto* menuBar = new QMenuBar(this);

    auto* fileMenu = menuBar->addMenu(tr("&File"));
    auto* reconnectAction = fileMenu->addAction(tr("Reconnect"));
    reconnectAction->setShortcut(QKeySequence::Refresh);
    connect(reconnectAction, &QAction::triggered, this, [this]() {
        if (m_ipcClient) {
            qInfo().noquote() << "[MainWindowGUI]" << "Manual reconnect triggered";
            m_ipcClient->connectToDaemon();
        }
    });
    fileMenu->addSeparator();
    auto* quitAction = fileMenu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    auto* toolsMenu = menuBar->addMenu(tr("&Tools"));
    auto* settingsAction = toolsMenu->addAction(tr("Settings..."));
    connect(settingsAction, &QAction::triggered, this, &MainWindowGUI::showSettingsDialog);

    auto* preferencesAction = toolsMenu->addAction(tr("Preferences..."));
    preferencesAction->setShortcut(QKeySequence(tr("Ctrl+,")));
    connect(preferencesAction, &QAction::triggered, this, &MainWindowGUI::showPreferencesDialog);

    toolsMenu->addSeparator();
    auto* logAction = toolsMenu->addAction(tr("Logs..."));
    connect(logAction, &QAction::triggered, this, &MainWindowGUI::showLogDialog);

    auto* investigateAction = toolsMenu->addAction(tr("Investigate..."));
    connect(investigateAction, &QAction::triggered, this, &MainWindowGUI::showInvestigateDialog);

    auto* configManagerAction = toolsMenu->addAction(tr("Manage Configurations..."));
    connect(configManagerAction, &QAction::triggered, this, &MainWindowGUI::showConfigManagerDialog);

    auto* notificationHistoryAction = toolsMenu->addAction(tr("Notification History..."));
    connect(notificationHistoryAction, &QAction::triggered, this, &MainWindowGUI::showNotificationHistoryDialog);

    auto* helpMenu = menuBar->addMenu(tr("&Help"));
    auto* shortcutsAction = helpMenu->addAction(tr("Keyboard Shortcuts..."));
    connect(shortcutsAction, &QAction::triggered, this, &MainWindowGUI::showKeyboardShortcutsDialog);

    auto* examplesAction = helpMenu->addAction(tr("Configuration Examples..."));
    connect(examplesAction, &QAction::triggered, this, &MainWindowGUI::showExamplesDialog);

    helpMenu->addSeparator();
    auto* aboutAction = helpMenu->addAction(tr("About YAMY..."));
    connect(aboutAction, &QAction::triggered, this, &MainWindowGUI::showAboutDialog);

    setMenuBar(menuBar);
}

void MainWindowGUI::showLogDialog() {
    if (!m_logDialog) {
        m_logDialog = new DialogLogQt(this);
        m_logDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_logDialog, &QObject::destroyed, this, [this]() { m_logDialog = nullptr; });
    }
    m_logDialog->show();
    m_logDialog->raise();
    m_logDialog->activateWindow();
}

void MainWindowGUI::showInvestigateDialog() {
    if (!m_investigateDialog) {
        m_investigateDialog = new DialogInvestigateQt(m_engine, this);
        m_investigateDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_investigateDialog, &QObject::destroyed, this, [this]() { m_investigateDialog = nullptr; });
    } else if (m_engine) {
        m_investigateDialog->setEngine(m_engine);
    }
    m_investigateDialog->show();
    m_investigateDialog->raise();
    m_investigateDialog->activateWindow();
}

void MainWindowGUI::showSettingsDialog() {
    if (!m_settingsDialog) {
        m_settingsDialog = new DialogSettingsQt(this);
        m_settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_settingsDialog, &QObject::destroyed, this, [this]() { m_settingsDialog = nullptr; });
    }
    m_settingsDialog->show();
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
}

void MainWindowGUI::showPreferencesDialog() {
    if (!m_preferencesDialog) {
        m_preferencesDialog = new PreferencesDialog(this);
        m_preferencesDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_preferencesDialog, &QObject::destroyed, this, [this]() { m_preferencesDialog = nullptr; });
    }
    m_preferencesDialog->show();
    m_preferencesDialog->raise();
    m_preferencesDialog->activateWindow();
}

void MainWindowGUI::showAboutDialog() {
    if (!m_aboutDialog) {
        m_aboutDialog = new DialogAboutQt(this);
        m_aboutDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_aboutDialog, &QObject::destroyed, this, [this]() { m_aboutDialog = nullptr; });
    }
    m_aboutDialog->show();
    m_aboutDialog->raise();
    m_aboutDialog->activateWindow();
}

void MainWindowGUI::showKeyboardShortcutsDialog() {
    if (!m_shortcutsDialog) {
        m_shortcutsDialog = new DialogShortcutsQt(this);
        m_shortcutsDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_shortcutsDialog, &QObject::destroyed, this, [this]() { m_shortcutsDialog = nullptr; });
    }
    m_shortcutsDialog->show();
    m_shortcutsDialog->raise();
    m_shortcutsDialog->activateWindow();
}

void MainWindowGUI::showExamplesDialog() {
    if (!m_examplesDialog) {
        m_examplesDialog = new DialogExamplesQt(this);
        m_examplesDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_examplesDialog, &QObject::destroyed, this, [this]() { m_examplesDialog = nullptr; });
    }
    m_examplesDialog->show();
    m_examplesDialog->raise();
    m_examplesDialog->activateWindow();
}

void MainWindowGUI::showConfigManagerDialog() {
    if (!m_configManagerDialog) {
        m_configManagerDialog = new ConfigManagerDialog(this);
        m_configManagerDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_configManagerDialog, &QObject::destroyed, this, [this]() { m_configManagerDialog = nullptr; });
    }
    m_configManagerDialog->show();
    m_configManagerDialog->raise();
    m_configManagerDialog->activateWindow();
}

void MainWindowGUI::showNotificationHistoryDialog() {
    if (!m_notificationHistoryDialog) {
        m_notificationHistoryDialog = new yamy::ui::NotificationHistoryDialog(this);
        m_notificationHistoryDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_notificationHistoryDialog, &QObject::destroyed, this, [this]() { m_notificationHistoryDialog = nullptr; });
    }
    m_notificationHistoryDialog->show();
    m_notificationHistoryDialog->raise();
    m_notificationHistoryDialog->activateWindow();
}
