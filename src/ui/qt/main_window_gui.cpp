#include "main_window_gui.h"

#include <QDebug>
#include <QLabel>
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
      m_toggleButton(new QPushButton(this)),
      m_hasStatus(false),
      m_currentEnabled(false) {
    setWindowTitle(tr("YAMY GUI (Prototype)"));

    auto* centralWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(centralWidget);
    m_connectionLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_configLabel->setAlignment(Qt::AlignCenter);
    m_toggleButton->setEnabled(false);
    layout->addWidget(m_connectionLabel);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_configLabel);
    layout->addWidget(m_toggleButton);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    updateStatusLabel(tr("Connecting to daemon..."));
    m_statusLabel->setText(tr("Status: unknown"));
    m_configLabel->setText(tr("Active config: -"));
    m_toggleButton->setText(tr("Enable"));

    connect(m_ipcClient.get(), &IPCClientGUI::connectionStateChanged, this, &MainWindowGUI::handleConnectionChange);
    connect(m_ipcClient.get(), &IPCClientGUI::statusReceived, this, &MainWindowGUI::handleStatusReceived);
    connect(m_toggleButton, &QPushButton::clicked, this, &MainWindowGUI::handleToggleClicked);

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
    updateStatusLabel(connected ? tr("Connected to daemon") : tr("Disconnected from daemon"));
    m_toggleButton->setEnabled(connected && m_hasStatus);
    if (!connected) {
        m_hasStatus = false;
        m_statusLabel->setText(tr("Status: unknown"));
        m_configLabel->setText(tr("Active config: -"));
        m_toggleButton->setText(tr("Enable"));
    }
    if (connected && m_ipcClient) {
        m_ipcClient->sendGetStatus();
    }
}

void MainWindowGUI::handleStatusReceived(const yamy::RspStatusPayload& payload) {
    const QString enabledText = payload.enabled ? tr("Enabled") : tr("Disabled");
    const QString configText = QString::fromLatin1(payload.activeConfig.data());

    m_hasStatus = true;
    m_currentEnabled = payload.enabled;

    updateStatusLabel(tr("Connected (%1)").arg(enabledText));
    m_statusLabel->setText(tr("Status: %1").arg(enabledText));
    m_configLabel->setText(configText.isEmpty() ? tr("Active config: (none)") : tr("Active config: %1").arg(configText));
    m_toggleButton->setText(payload.enabled ? tr("Disable") : tr("Enable"));
    m_toggleButton->setEnabled(m_ipcClient && m_ipcClient->isConnected());
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

void MainWindowGUI::updateStatusLabel(const QString& text) {
    if (m_connectionLabel) {
        m_connectionLabel->setText(text);
    }
    qInfo().noquote() << "[MainWindowGUI]" << text;
}
