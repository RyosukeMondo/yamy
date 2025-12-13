#include "dialog_examples_qt.h"
#include <QClipboard>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QFontDatabase>
#include <QLabel>
#include <QTimer>
#include <QDir>
#include <QTextStream>

DialogExamplesQt::DialogExamplesQt(QWidget* parent)
    : QDialog(parent)
    , m_exampleList(nullptr)
    , m_codeView(nullptr)
    , m_btnCopy(nullptr)
    , m_btnSaveAs(nullptr)
    , m_btnClose(nullptr)
{
    setWindowTitle("Configuration Examples");
    setMinimumSize(700, 500);
    resize(800, 600);

    setupUI();
    loadExamples();
}

DialogExamplesQt::~DialogExamplesQt()
{
}

void DialogExamplesQt::onExampleSelected(int index)
{
    if (index >= 0) {
        m_codeView->setPlainText(getExampleCode(index));
        m_btnCopy->setEnabled(true);
        m_btnSaveAs->setEnabled(true);
    } else {
        m_codeView->clear();
        m_btnCopy->setEnabled(false);
        m_btnSaveAs->setEnabled(false);
    }
}

void DialogExamplesQt::onCopyToClipboard()
{
    QString code = m_codeView->toPlainText();
    if (!code.isEmpty()) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(code);

        // Show brief visual feedback
        QString originalText = m_btnCopy->text();
        m_btnCopy->setText("Copied!");
        m_btnCopy->setEnabled(false);

        // Reset button after short delay
        QTimer::singleShot(1500, this, [this, originalText]() {
            m_btnCopy->setText(originalText);
            m_btnCopy->setEnabled(true);
        });
    }
}

void DialogExamplesQt::onSaveAs()
{
    QString code = m_codeView->toPlainText();
    if (code.isEmpty()) {
        return;
    }

    QString defaultName = m_exampleList->currentItem()
        ? m_exampleList->currentItem()->text().toLower().replace(" ", "_") + ".mayu"
        : "example.mayu";

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Configuration Example",
        QDir::homePath() + "/.config/yamy/" + defaultName,
        "YAMY Configuration (*.mayu);;All Files (*)"
    );

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << code;
            file.close();

            QMessageBox::information(
                this,
                "Example Saved",
                QString("Configuration saved to:\n%1").arg(fileName)
            );
        } else {
            QMessageBox::warning(
                this,
                "Save Failed",
                QString("Could not save to:\n%1\n\nError: %2")
                    .arg(fileName, file.errorString())
            );
        }
    }
}

void DialogExamplesQt::onClose()
{
    close();
}

void DialogExamplesQt::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Description label
    QLabel* descLabel = new QLabel(
        "Select an example from the list to view its configuration code. "
        "You can copy the code or save it as a new configuration file."
    );
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #666; margin-bottom: 10px;");
    mainLayout->addWidget(descLabel);

    // Splitter for list and code view
    QSplitter* splitter = new QSplitter(Qt::Horizontal);

    // Example list
    m_exampleList = new QListWidget();
    m_exampleList->setMaximumWidth(200);
    connect(m_exampleList, &QListWidget::currentRowChanged,
            this, &DialogExamplesQt::onExampleSelected);
    splitter->addWidget(m_exampleList);

    // Code view with monospace font
    m_codeView = new QTextEdit();
    m_codeView->setReadOnly(true);
    m_codeView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_codeView->setLineWrapMode(QTextEdit::NoWrap);
    splitter->addWidget(m_codeView);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    mainLayout->addWidget(splitter);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();

    m_btnCopy = new QPushButton("Copy to Clipboard");
    m_btnCopy->setEnabled(false);
    connect(m_btnCopy, &QPushButton::clicked, this, &DialogExamplesQt::onCopyToClipboard);
    btnLayout->addWidget(m_btnCopy);

    m_btnSaveAs = new QPushButton("Save As...");
    m_btnSaveAs->setEnabled(false);
    connect(m_btnSaveAs, &QPushButton::clicked, this, &DialogExamplesQt::onSaveAs);
    btnLayout->addWidget(m_btnSaveAs);

    btnLayout->addStretch();

    m_btnClose = new QPushButton("Close");
    m_btnClose->setDefault(true);
    connect(m_btnClose, &QPushButton::clicked, this, &DialogExamplesQt::onClose);
    btnLayout->addWidget(m_btnClose);

    mainLayout->addLayout(btnLayout);
}

void DialogExamplesQt::loadExamples()
{
    m_exampleList->addItem("Basic Remapping");
    m_exampleList->addItem("Emacs Bindings");
    m_exampleList->addItem("Vim Modal Editing");
    m_exampleList->addItem("CapsLock as Ctrl");
    m_exampleList->addItem("Window Management");

    // Select first item
    m_exampleList->setCurrentRow(0);
}

QString DialogExamplesQt::getExampleCode(int index) const
{
    switch (index) {
        case 0:  // Basic Remapping
            return R"(# Basic Key Remapping Example
# ===========================
# This example shows basic key remapping for YAMY.

# Define a global keymap that applies everywhere
keymap Global

    # Swap Caps Lock and Left Control
    # Many programmers prefer Ctrl on the home row
    key CapsLock = Control
    key Control = CapsLock

    # Remap Right Alt to be a compose key (on Linux)
    # This is useful for typing special characters
    # key RAlt = Compose

    # Map unused keys to something useful
    # key Pause = MediaPlayPause
    # key ScrollLock = Mute
)";

        case 1:  // Emacs Bindings
            return R"(# Emacs-style Navigation Bindings
# ================================
# Navigate text using Ctrl key combinations.
# Works in most applications.

keymap Global

    # Basic cursor movement (like Emacs)
    key C-f = Right        # Forward character
    key C-b = Left         # Backward character
    key C-n = Down         # Next line
    key C-p = Up           # Previous line

    # Word movement
    key A-f = C-Right      # Forward word
    key A-b = C-Left       # Backward word

    # Line navigation
    key C-a = Home         # Beginning of line
    key C-e = End          # End of line

    # Page navigation
    key C-v = PageDown     # Page down
    key A-v = PageUp       # Page up

    # Deletion
    key C-d = Delete       # Delete forward
    key C-h = BackSpace    # Delete backward
    key A-d = C-Delete     # Delete word forward
    key A-h = C-BackSpace  # Delete word backward

    # Clipboard (standard keys for compatibility)
    # key C-w = C-x        # Cut (conflicts with Ctrl-W close)
    key A-w = C-c          # Copy
    key C-y = C-v          # Paste
)";

        case 2:  // Vim Modal Editing
            return R"(# Vim-style Modal Editing
# ======================
# Use CapsLock to toggle between normal and insert modes.
# In normal mode, hjkl work as arrow keys.

# Normal mode keymap (navigation)
keymap VimNormal

    # Basic movement (hjkl)
    key h = Left
    key j = Down
    key k = Up
    key l = Right

    # Word movement
    key w = C-Right        # Next word
    key b = C-Left         # Previous word

    # Line operations
    key 0 = Home           # Beginning of line
    key $ = End            # End of line

    # Enter insert mode
    key i = &VimInsert     # Insert before cursor
    key a = Right &VimInsert  # Append after cursor

    # Copy/paste (y = yank, p = paste)
    key y = C-c            # Yank (copy)
    key p = C-v            # Paste
    key d = C-x            # Delete (cut)

    # Undo/redo
    key u = C-z            # Undo

# Insert mode keymap (typing)
keymap VimInsert

    # Exit insert mode with Escape or CapsLock
    key Escape = &VimNormal
    key CapsLock = &VimNormal

# Start in insert mode by default
keymap Global : VimInsert
)";

        case 3:  // CapsLock as Ctrl
            return R"(# CapsLock as Control Key
# =======================
# A simple but powerful modification.
# Makes CapsLock act as Control when held with other keys,
# and as Escape when tapped alone.

keymap Global

    # Option 1: Simple swap - CapsLock becomes Control
    key CapsLock = Control

    # Option 2: Dual-function (commented out)
    # When held: acts as Control
    # When tapped: acts as Escape
    # key CapsLock = &ControlOrEscape

    # If you still need CapsLock occasionally:
    # Use Shift+CapsLock to toggle caps
    # key S-CapsLock = CapsLock

# Uncomment for dual-function behavior:
# keymap ControlOrEscape
#     # Tap CapsLock alone = Escape
#     key -CapsLock = Escape
#     # Hold CapsLock = Control modifier active
#     mod Control = CapsLock
)";

        case 4:  // Window Management
            return R"(# Window Management Shortcuts
# ===========================
# Custom shortcuts for window management.
# These work with common Linux desktop environments.

keymap Global

    # Window snapping (like Windows Aero Snap)
    # Note: Actual behavior depends on your desktop environment
    key Win-Left = &SnapLeft
    key Win-Right = &SnapRight
    key Win-Up = &Maximize
    key Win-Down = &Minimize

    # Virtual desktop navigation
    key Win-1 = C-A-1      # Go to desktop 1
    key Win-2 = C-A-2      # Go to desktop 2
    key Win-3 = C-A-3      # Go to desktop 3
    key Win-4 = C-A-4      # Go to desktop 4

    # Move window to desktop
    key Win-S-1 = C-A-S-1  # Move window to desktop 1
    key Win-S-2 = C-A-S-2  # Move window to desktop 2

    # Quick application launchers
    key Win-t = &LaunchTerminal
    key Win-e = &LaunchFileManager
    key Win-b = &LaunchBrowser

# Snap left: resize to left half of screen
keymap SnapLeft
    key = Super-Left

# Snap right: resize to right half of screen
keymap SnapRight
    key = Super-Right

# Maximize window
keymap Maximize
    key = Super-Up

# Minimize/restore window
keymap Minimize
    key = Super-Down

# Application launchers (adjust commands for your system)
keymap LaunchTerminal
    key = C-A-t            # Common terminal shortcut

keymap LaunchFileManager
    key = Super-e          # Common file manager shortcut

keymap LaunchBrowser
    key = &Spawn("firefox")  # Launch Firefox
)";

        default:
            return "# No example selected";
    }
}
