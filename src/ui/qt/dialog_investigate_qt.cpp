#include "dialog_investigate_qt.h"
#include "crosshair_widget_qt.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFont>
#include <QDateTime>

DialogInvestigateQt::DialogInvestigateQt(QWidget* parent)
    : QDialog(parent)
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

    // Note: Process info (m_labelProcess, m_labelProcessPath) is Task 3.4
}
