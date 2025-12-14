#include "main_window_gui.h"

#include <QAction>
#include <QComboBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "ipc_client_gui.h"

MainWindowGUI::MainWindowGUI(const QString& serverName, QWidget* parent)
    : QMainWindow(parent),
      m_ipcClient(std::make_unique<IPCClientGUI>(this)),
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
      m_isConnected(false) {
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
    m_configSelector->setPlaceholderText(tr("Config selector (coming soon)"));
    configRow->addWidget(m_configSelector, /*stretch*/ 1);
    m_reloadButton->setText(tr("Reload"));
    configRow->addWidget(m_reloadButton, /*stretch*/ 0);
    layout->addLayout(configRow);

    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    updateStatusLabel(tr("Connecting to daemon..."));
    m_statusLabel->setText(tr("Status: unknown"));
    m_configLabel->setText(tr("Active config: -"));
    m_toggleButton->setText(tr("Enable"));
    updateIndicators(false, false, QString());

    connect(m_ipcClient.get(), &IPCClientGUI::connectionStateChanged, this, &MainWindowGUI::handleConnectionChange);
    connect(m_ipcClient.get(), &IPCClientGUI::statusReceived, this, &MainWindowGUI::handleStatusReceived);
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
    if (!connected) {
        m_hasStatus = false;
        m_activeConfig.clear();
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

    m_hasStatus = true;
    m_currentEnabled = payload.enabled;
    m_activeConfig = configText;

    updateStatusLabel(tr("Connected (%1)").arg(enabledText));
    m_statusLabel->setText(tr("Status: %1").arg(enabledText));
    m_configLabel->setText(configText.isEmpty() ? tr("Active config: (none)") : tr("Active config: %1").arg(configText));
    m_toggleButton->setText(payload.enabled ? tr("Disable") : tr("Enable"));
    m_toggleButton->setEnabled(m_ipcClient && m_ipcClient->isConnected());
    m_configSelector->setEnabled(m_ipcClient && m_ipcClient->isConnected());
    m_reloadButton->setEnabled(m_ipcClient && m_ipcClient->isConnected());
    updateIndicators(m_isConnected, m_currentEnabled, m_activeConfig);
}

void MainWindowGUI::handleToggleClicked() {
    if (!m_ipcClient || !m_hasStatus) {
        return;
    }
    const bool targetEnabled = !m_currentEnabled;
    m_toggleButton->setEnabled(false);
    m_toggleButton->setText(targetEnabled ? tr("Enabling...") : tr("Disabling..."));
    m_ipcClient->sendSetEnabled(targetEnabled);
}

void MainWindowGUI::handleReloadClicked() {
    // Placeholder for upcoming configuration management behavior.
    qInfo().noquote() << "[MainWindowGUI]" << "Reload requested (awaiting config wiring)";
}

void MainWindowGUI::handleConfigSelectionChanged(const QString& configName) {
    // Placeholder for upcoming configuration management behavior.
    qInfo().noquote() << "[MainWindowGUI]" << "Config selection changed to" << configName;
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
    auto* menu = new QMenuBar(this);

    auto* fileMenu = menu->addMenu(tr("&File"));
    auto* reconnectAction = fileMenu->addAction(tr("Reconnect"));
    reconnectAction->setShortcut(QKeySequence::Refresh);
    connect(reconnectAction, &QAction::triggered, this, [this]() {
        if (m_ipcClient) {
            qInfo().noquote() << "[MainWindowGUI]" << "Manual reconnect triggered";
            m_ipcClient->connectToDaemon();
        }
    });
    auto* quitAction = fileMenu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    auto* viewMenu = menu->addMenu(tr("&View"));
    viewMenu->addAction(tr("Status Panel"))->setEnabled(false);
    viewMenu->addAction(tr("Config Panel"))->setEnabled(false);

    auto* toolsMenu = menu->addMenu(tr("&Tools"));
    toolsMenu->addAction(tr("Logs"))->setEnabled(false);
    toolsMenu->addAction(tr("Investigate"))->setEnabled(false);

    auto* helpMenu = menu->addMenu(tr("&Help"));
    helpMenu->addAction(tr("About"))->setEnabled(false);

    setMenuBar(menu);
}
