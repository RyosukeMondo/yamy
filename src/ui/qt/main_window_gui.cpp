#include "main_window_gui.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include "ipc_client_gui.h"

MainWindowGUI::MainWindowGUI(QWidget* parent)
    : QMainWindow(parent),
      m_ipcClient(std::make_unique<IPCClientGUI>(this)),
      m_connectionLabel(new QLabel(this)) {
    setWindowTitle(tr("YAMY GUI (Prototype)"));

    auto* centralWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(centralWidget);
    m_connectionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_connectionLabel);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    updateStatusLabel(tr("Connecting to daemon..."));

    connect(m_ipcClient.get(), &IPCClientGUI::connectionStateChanged, this, &MainWindowGUI::handleConnectionChange);
    connect(m_ipcClient.get(), &IPCClientGUI::statusReceived, this, &MainWindowGUI::handleStatusReceived);

    m_ipcClient->connectToDaemon();
}

MainWindowGUI::~MainWindowGUI() {
    if (m_ipcClient) {
        m_ipcClient->disconnectFromDaemon();
    }
}

void MainWindowGUI::handleConnectionChange(bool connected) {
    updateStatusLabel(connected ? tr("Connected to daemon") : tr("Disconnected from daemon"));
    if (connected && m_ipcClient) {
        m_ipcClient->sendGetStatus();
    }
}

void MainWindowGUI::handleStatusReceived(const yamy::RspStatusPayload& payload) {
    const QString enabledText = payload.enabled ? tr("Enabled") : tr("Disabled");
    updateStatusLabel(tr("Connected (%1)").arg(enabledText));
}

void MainWindowGUI::updateStatusLabel(const QString& text) {
    if (m_connectionLabel) {
        m_connectionLabel->setText(text);
    }
}
