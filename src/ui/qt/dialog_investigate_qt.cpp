#include "dialog_investigate_qt.h"
#include "crosshair_widget_qt.h"
#include "dialog_condition_generator_qt.h"
#include "../../core/engine/engine.h"
#include "../../core/platform/ipc_channel_factory.h"
#include "../../core/platform/window_system_interface.h"
#include "../../core/ipc_messages.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QGroupBox>
#include <QDateTime>
#include <QTimer>
#include <QHeaderView>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QClipboard>
#include <QApplication>

#ifndef _WIN32
#include <unistd.h>
#endif
#include <limits.h>

DialogInvestigateQt::DialogInvestigateQt(Engine* engine, QWidget* parent)
    : QDialog(parent)
    , m_engine(engine)
    , m_windowSystem(yamy::platform::createWindowSystem())
    , m_crosshair(nullptr)
    , m_labelHandle(nullptr)
    , m_labelTitle(nullptr)
    , m_labelClass(nullptr)
    , m_labelProcess(nullptr)
    , m_labelProcessPath(nullptr)
    , m_labelGeometry(nullptr)
    , m_labelState(nullptr)
    , m_labelKeymapName(nullptr)
    , m_labelMatchedRegex(nullptr)
    , m_labelModifiers(nullptr)
    , m_liveLog(nullptr)
    , m_btnSelectWindow(nullptr)
    , m_btnClose(nullptr)
    , m_selectedWindow(nullptr)
{
    setWindowTitle("Investigate Window");
    setMinimumSize(800, 600);
    resize(800, 600);

    // Make non-modal so user can interact with other windows
    setModal(false);

    setupUI();

    // Setup IPC channel
    m_ipcChannel = yamy::platform::createIPCChannel("yamy-investigate");
    if (m_ipcChannel) {
        connect(m_ipcChannel.get(), &yamy::platform::IIPCChannel::messageReceived,
                this, &DialogInvestigateQt::onIpcMessageReceived);
        m_ipcChannel->connect("yamy-engine");
    }
}

DialogInvestigateQt::~DialogInvestigateQt()
{
    // Ensure crosshair is deactivated
    if (m_crosshair && m_crosshair->isActive()) {
        m_crosshair->deactivate();
    }
}

void DialogInvestigateQt::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Create crosshair widget (not visible initially)
    m_crosshair = new CrosshairWidget(nullptr);  // Parent is null for top-level overlay
    connect(m_crosshair, &CrosshairWidget::windowSelected,
            this, &DialogInvestigateQt::onWindowSelected);
    connect(m_crosshair, &CrosshairWidget::selectionCancelled,
            this, &DialogInvestigateQt::onSelectionCancelled);

    // Top section: Select Window button
    QHBoxLayout* topLayout = new QHBoxLayout();

    m_btnSelectWindow = new QPushButton("Select Window");
    m_btnSelectWindow->setToolTip("Click and drag crosshair to select a window");
    m_btnSelectWindow->setMinimumWidth(150);
    connect(m_btnSelectWindow, &QPushButton::clicked,
            this, &DialogInvestigateQt::onSelectWindow);
    topLayout->addWidget(m_btnSelectWindow);

    topLayout->addStretch();
    mainLayout->addLayout(topLayout);

    // Middle section: Info panels side by side
    QHBoxLayout* panelsLayout = new QHBoxLayout();

    // Left side: Window info panel
    panelsLayout->addWidget(createWindowInfoPanel());

    // Right side: Keymap status panel
    panelsLayout->addWidget(createKeymapStatusPanel());

    mainLayout->addLayout(panelsLayout);

    // Bottom section: Live log panel
    mainLayout->addWidget(createLiveLogPanel(), 1);  // Give it stretch factor

    // Dialog buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_btnCopyToClipboard = new QPushButton("Copy to Clipboard");
    connect(m_btnCopyToClipboard, &QPushButton::clicked, this, &DialogInvestigateQt::onCopyToClipboard);
    buttonLayout->addWidget(m_btnCopyToClipboard);

    m_btnGenerateCondition = new QPushButton("Generate Condition");
    connect(m_btnGenerateCondition, &QPushButton::clicked, this, &DialogInvestigateQt::onGenerateCondition);
    buttonLayout->addWidget(m_btnGenerateCondition);

    m_btnClose = new QPushButton("Close");
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::close);
    buttonLayout->addWidget(m_btnClose);

    mainLayout->addLayout(buttonLayout);
}

QGroupBox* DialogInvestigateQt::createWindowInfoPanel()
{
    QGroupBox* group = new QGroupBox("Window Information");
    QGridLayout* layout = new QGridLayout(group);
    layout->setColumnStretch(1, 1);

    int row = 0;

    // Handle
    layout->addWidget(new QLabel("Handle:"), row, 0);
    m_labelHandle = new QLabel("-");
    m_labelHandle->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_labelHandle, row++, 1);

    // Title
    layout->addWidget(new QLabel("Title:"), row, 0);
    m_labelTitle = new QLabel("-");
    m_labelTitle->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_labelTitle->setWordWrap(true);
    layout->addWidget(m_labelTitle, row++, 1);

    // Class name
    layout->addWidget(new QLabel("Class:"), row, 0);
    m_labelClass = new QLabel("-");
    m_labelClass->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_labelClass, row++, 1);

    // Process name
    layout->addWidget(new QLabel("Process:"), row, 0);
    m_labelProcess = new QLabel("-");
    m_labelProcess->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_labelProcess, row++, 1);

    // Process path
    layout->addWidget(new QLabel("Path:"), row, 0);
    m_labelProcessPath = new QLabel("-");
    m_labelProcessPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_labelProcessPath->setWordWrap(true);
    layout->addWidget(m_labelProcessPath, row++, 1);

    // Geometry
    layout->addWidget(new QLabel("Geometry:"), row, 0);
    m_labelGeometry = new QLabel("-");
    m_labelGeometry->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_labelGeometry, row++, 1);

    // State
    layout->addWidget(new QLabel("State:"), row, 0);
    m_labelState = new QLabel("-");
    m_labelState->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_labelState, row++, 1);

    // Add stretch at the bottom
    layout->setRowStretch(row, 1);

    return group;
}

QGroupBox* DialogInvestigateQt::createKeymapStatusPanel()
{
    QGroupBox* group = new QGroupBox("Keymap Status");
    QGridLayout* layout = new QGridLayout(group);
    layout->setColumnStretch(1, 1);

    int row = 0;

    // Active keymap
    layout->addWidget(new QLabel("Keymap:"), row, 0);
    m_labelKeymapName = new QLabel("-");
    m_labelKeymapName->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_labelKeymapName, row++, 1);

    // Matched regex
    layout->addWidget(new QLabel("Matched Regex:"), row, 0);
    m_labelMatchedRegex = new QLabel("-");
    m_labelMatchedRegex->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_labelMatchedRegex->setWordWrap(true);
    layout->addWidget(m_labelMatchedRegex, row++, 1);

    // Active modifiers
    layout->addWidget(new QLabel("Modifiers:"), row, 0);
    m_labelModifiers = new QLabel("-");
    m_labelModifiers->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_labelModifiers, row++, 1);

    // Add stretch at the bottom
    layout->setRowStretch(row, 1);

    return group;
}

QGroupBox* DialogInvestigateQt::createLiveLogPanel()
{
    QGroupBox* group = new QGroupBox("Live Key Events");

    QVBoxLayout* layout = new QVBoxLayout(group);

    m_liveLog = new QTextEdit();
    m_liveLog->setReadOnly(true);
    m_liveLog->setPlaceholderText("Key events will appear here when a window is selected...");

    // Use monospace font for log
    QFont monoFont("monospace");
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(9);
    m_liveLog->setFont(monoFont);

    layout->addWidget(m_liveLog);

    return group;
}

void DialogInvestigateQt::onSelectWindow()
{
    // Hide the dialog while selecting
    hide();

    // Activate crosshair overlay
    m_crosshair->activate();
}

void DialogInvestigateQt::onWindowSelected(yamy::platform::WindowHandle hwnd)
{
    // Show the dialog again
    show();
    raise();
    activateWindow();

    // Store the selected window
    m_selectedWindow = hwnd;

    // Clear previous info
    clearPanels();

    // Log selection
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    if (hwnd) {
        m_liveLog->append(QString("[%1] Window selected: 0x%2")
            .arg(timestamp)
            .arg(reinterpret_cast<quintptr>(hwnd), 0, 16));

        // Retrieve and display full window information
        updateWindowInfo(hwnd);
    } else {
        m_labelHandle->setText("(none)");
        m_liveLog->append(QString("[%1] No window selected").arg(timestamp));
    }

    emit windowInvestigated(hwnd);
}

void DialogInvestigateQt::onSelectionCancelled()
{
    // Show the dialog again
    show();
    raise();
    activateWindow();

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    m_liveLog->append(QString("[%1] Selection cancelled").arg(timestamp));
}

void DialogInvestigateQt::clearPanels()
{
    // Clear window info
    m_labelHandle->setText("-");
    m_labelTitle->setText("-");
    m_labelClass->setText("-");
    m_labelProcess->setText("-");
    m_labelProcessPath->setText("-");
    m_labelGeometry->setText("-");
    m_labelState->setText("-");

    // Clear keymap status
    m_labelKeymapName->setText("-");
    m_labelMatchedRegex->setText("-");
    m_labelModifiers->setText("-");
}

void DialogInvestigateQt::updateWindowInfo(yamy::platform::WindowHandle hwnd)
{
    if (!hwnd || !m_windowSystem) {
        m_labelHandle->setText("(invalid)");
        return;
    }

    // Display window handle in hex format
    m_labelHandle->setText(QString("0x%1").arg(
        reinterpret_cast<quintptr>(hwnd), 0, 16));

    // Get window title
    std::string title = m_windowSystem->getWindowText(hwnd);
    if (!title.empty()) {
        m_labelTitle->setText(QString::fromUtf8(title.c_str()));
    } else {
        m_labelTitle->setText("(no title)");
    }

    // Get class name
    std::string className = m_windowSystem->getClassName(hwnd);
    if (!className.empty()) {
        m_labelClass->setText(QString::fromUtf8(className.c_str()));
    } else {
        m_labelClass->setText("(unknown)");
    }

    // Get window geometry
    yamy::platform::Rect rect;
    if (m_windowSystem->getWindowRect(hwnd, &rect)) {
        m_labelGeometry->setText(QString("%1, %2  %3x%4")
            .arg(rect.left)
            .arg(rect.top)
            .arg(rect.width())
            .arg(rect.height()));
    } else {
        m_labelGeometry->setText("(unavailable)");
    }

    // Get window state
    yamy::platform::WindowShowCmd showCmd = m_windowSystem->getShowCommand(hwnd);
    QString stateText;
    switch (showCmd) {
        case yamy::platform::WindowShowCmd::Normal:
            stateText = "Normal";
            break;
        case yamy::platform::WindowShowCmd::Maximized:
            stateText = "Maximized";
            break;
        case yamy::platform::WindowShowCmd::Minimized:
            stateText = "Minimized";
            break;
        default:
            stateText = "Unknown";
            break;
    }
    m_labelState->setText(stateText);

    // Get process information using PID
    uint32_t pid = m_windowSystem->getWindowProcessId(hwnd);
    if (pid > 0) {
        QString processName = getProcessName(pid);
        if (!processName.isEmpty()) {
            m_labelProcess->setText(processName);
        } else {
            m_labelProcess->setText(QString("(PID: %1)").arg(pid));
        }

        QString processPath = getProcessPath(pid);
        if (!processPath.isEmpty()) {
            m_labelProcessPath->setText(processPath);
        } else {
            m_labelProcessPath->setText("(unavailable)");
        }
    } else {
        m_labelProcess->setText("(unknown)");
        m_labelProcessPath->setText("(unavailable)");
    }

    // Get class name and title for keymap query
    std::string classNameStr = m_windowSystem->getClassName(hwnd);
    std::string titleNameStr = m_windowSystem->getWindowText(hwnd);

    // Update keymap status panel
    updateKeymapStatus(hwnd, classNameStr, titleNameStr);
}

QString DialogInvestigateQt::getProcessName(uint32_t pid)
{
    if (pid == 0) {
        return QString();
    }

#ifndef _WIN32
    // Read process name from /proc/{pid}/comm
    QString commPath = QString("/proc/%1/comm").arg(pid);
    QFile commFile(commPath);

    if (!commFile.exists()) {
        return QString();
    }

    if (!commFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream stream(&commFile);
    QString processName = stream.readLine().trimmed();
    commFile.close();

    return processName;
#else
    return QString(); // TODO: Windows implementation
#endif
}

QString DialogInvestigateQt::getProcessPath(uint32_t pid)
{
    if (pid == 0) {
        return QString();
    }

#ifndef _WIN32
    // Read executable path from /proc/{pid}/exe using readlink
    QString exePath = QString("/proc/%1/exe").arg(pid);

    // Check if the symlink exists
    QFileInfo fileInfo(exePath);
    if (!fileInfo.exists()) {
        return QString();
    }

    // Use readlink to resolve the symlink
    char buffer[PATH_MAX];
    std::string pathStr = exePath.toStdString();
    ssize_t len = readlink(pathStr.c_str(), buffer, sizeof(buffer) - 1);

    if (len == -1) {
        // readlink failed (permission denied or other error)
        return QString();
    }

    buffer[len] = '\0';
    return QString::fromUtf8(buffer);
#else
    return QString(); // TODO: Windows implementation
#endif
}

void DialogInvestigateQt::setEngine(Engine* engine)
{
    m_engine = engine;
}

void DialogInvestigateQt::updateKeymapStatus(
    yamy::platform::WindowHandle hwnd,
    const std::string& className,
    const std::string& titleName)
{
    if (!m_ipcChannel || !m_ipcChannel->isConnected()) {
        m_labelKeymapName->setText("(IPC not connected)");
        return;
    }

    yamy::ipc::InvestigateWindowRequest request;
    request.hwnd = hwnd;

    yamy::ipc::Message message;
    message.type = yamy::ipc::CmdInvestigateWindow;
    message.data = &request;
    message.size = sizeof(request);

    m_ipcChannel->send(message);
}

void DialogInvestigateQt::onIpcMessageReceived(const yamy::ipc::Message& message)
{
    if (message.type == yamy::ipc::RspInvestigateWindow) {
        if (message.size >= sizeof(yamy::ipc::InvestigateWindowResponse)) {
            const auto* response = static_cast<const yamy::ipc::InvestigateWindowResponse*>(message.data);

            m_labelKeymapName->setText(QString::fromUtf8(response->keymapName));
            m_labelModifiers->setText(QString::fromUtf8(response->activeModifiers));

            QString regexText;
            if (response->matchedClassRegex[0] != '\0' && strcmp(response->matchedClassRegex, ".*") != 0) {
                regexText = QString("Class: /%1/").arg(QString::fromUtf8(response->matchedClassRegex));
            }
            if (response->matchedTitleRegex[0] != '\0' && strcmp(response->matchedTitleRegex, ".*") != 0) {
                if (!regexText.isEmpty()) {
                    regexText += "\n";
                }
                regexText += QString("Title: /%1/").arg(QString::fromUtf8(response->matchedTitleRegex));
            }

            if (regexText.isEmpty()) {
                if (response->isDefault) {
                    regexText = "(global keymap)";
                } else {
                    regexText = "(no pattern)";
                }
            }
            m_labelMatchedRegex->setText(regexText);
        }
    } else if (message.type == yamy::ipc::NtfKeyEvent) {
        if (message.size >= sizeof(yamy::ipc::KeyEventNotification)) {
            const auto* notification = static_cast<const yamy::ipc::KeyEventNotification*>(message.data);
            m_liveLog->append(QString::fromUtf8(notification->keyEvent));
        }
    }
}

void DialogInvestigateQt::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    if (m_ipcChannel && m_ipcChannel->isConnected()) {
        yamy::ipc::Message message;
        message.type = yamy::ipc::CmdEnableInvestigateMode;
        m_ipcChannel->send(message);
    }
}

void DialogInvestigateQt::hideEvent(QHideEvent* event)
{
    QDialog::hideEvent(event);
    if (m_ipcChannel && m_ipcChannel->isConnected()) {
        yamy::ipc::Message message;
        message.type = yamy::ipc::CmdDisableInvestigateMode;
        m_ipcChannel->send(message);
    }
}

#include <QClipboard>
#include <QApplication>

void DialogInvestigateQt::onCopyToClipboard()
{
    QString textToCopy;
    textToCopy += "Window Information\n";
    textToCopy += "------------------\n";
    textToCopy += "Handle: " + m_labelHandle->text() + "\n";
    textToCopy += "Title: " + m_labelTitle->text() + "\n";
    textToCopy += "Class: " + m_labelClass->text() + "\n";
    textToCopy += "Process: " + m_labelProcess->text() + "\n";
    textToCopy += "Path: " + m_labelProcessPath->text() + "\n";
    textToCopy += "Geometry: " + m_labelGeometry->text() + "\n";
    textToCopy += "State: " + m_labelState->text() + "\n";
    textToCopy += "\n";
    textToCopy += "Keymap Status\n";
    textToCopy += "-------------\n";
    textToCopy += "Keymap: " + m_labelKeymapName->text() + "\n";
    textToCopy += "Matched Regex: " + m_labelMatchedRegex->text() + "\n";
    textToCopy += "Modifiers: " + m_labelModifiers->text() + "\n";
    textToCopy += "\n";
    textToCopy += "Live Key Events\n";
    textToCopy += "---------------\n";
    textToCopy += m_liveLog->toPlainText();

    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(textToCopy);
}

void DialogInvestigateQt::onGenerateCondition()
{
    if (m_selectedWindow) {
        DialogConditionGeneratorQt dialog(m_labelTitle->text(), m_labelClass->text(), this);
        dialog.exec();
    }
}
