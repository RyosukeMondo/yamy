#include "dialog_settings_qt.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <cstdlib>

DialogSettingsQt::DialogSettingsQt(QWidget* parent)
    : QDialog(parent)
    , m_keymapList(nullptr)
    , m_btnAdd(nullptr)
    , m_btnEdit(nullptr)
    , m_btnRemove(nullptr)
    , m_btnBrowse(nullptr)
    , m_btnSave(nullptr)
    , m_btnCancel(nullptr)
    , m_editKeymapPath(nullptr)
    , m_editEditorCommand(nullptr)
    , m_btnBrowseEditor(nullptr)
    , m_labelStatus(nullptr)
{
    setWindowTitle("YAMY Settings");
    setMinimumSize(600, 400);

    setupUI();
    loadSettings();
}

DialogSettingsQt::~DialogSettingsQt()
{
}

QStringList DialogSettingsQt::getKeymapFiles() const
{
    return m_keymapFiles;
}

void DialogSettingsQt::setKeymapFiles(const QStringList& files)
{
    m_keymapFiles = files;
    m_keymapList->clear();
    m_keymapList->addItems(files);
}

void DialogSettingsQt::onAddKeymap()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        "Add Keymap File",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        "Keymap Files (*.mayu);;All Files (*)"
    );

    if (!file.isEmpty()) {
        if (!m_keymapFiles.contains(file)) {
            m_keymapFiles.append(file);
            m_keymapList->addItem(file);
            m_labelStatus->setText("Added: " + file);
        } else {
            QMessageBox::warning(this, "YAMY", "This keymap file is already in the list.");
        }
    }
}

void DialogSettingsQt::onEditKeymap()
{
    QListWidgetItem* item = m_keymapList->currentItem();
    if (!item) {
        return;
    }

    QString file = item->text();
    openInEditor(file);
}

void DialogSettingsQt::openInEditor(const QString& path)
{
    QSettings settings("YAMY", "YAMY");
    QString editorCmd = settings.value("editor/command", "").toString();

    // Strategy 1: Use configured editor command
    if (!editorCmd.isEmpty()) {
        if (launchEditor(editorCmd, path)) {
            m_labelStatus->setText("Opened in editor: " + QFileInfo(path).fileName());
            return;
        }
    }

    // Strategy 2: Try $EDITOR environment variable
    const char* envEditor = std::getenv("EDITOR");
    if (envEditor && strlen(envEditor) > 0) {
        QString envEditorCmd = QString::fromLocal8Bit(envEditor) + " %f";
        if (launchEditor(envEditorCmd, path)) {
            m_labelStatus->setText("Opened in editor: " + QFileInfo(path).fileName());
            return;
        }
    }

    // Strategy 3: Try $VISUAL environment variable
    const char* envVisual = std::getenv("VISUAL");
    if (envVisual && strlen(envVisual) > 0) {
        QString visualCmd = QString::fromLocal8Bit(envVisual) + " %f";
        if (launchEditor(visualCmd, path)) {
            m_labelStatus->setText("Opened in editor: " + QFileInfo(path).fileName());
            return;
        }
    }

    // Strategy 4: Use QDesktopServices
    QUrl fileUrl = QUrl::fromLocalFile(path);
    if (QDesktopServices::openUrl(fileUrl)) {
        m_labelStatus->setText("Opened in editor: " + QFileInfo(path).fileName());
        return;
    }

    // Strategy 5: Platform fallback
#ifdef Q_OS_LINUX
    QProcess process;
    process.setProgram("xdg-open");
    process.setArguments({path});
    if (process.startDetached()) {
        m_labelStatus->setText("Opened in editor: " + QFileInfo(path).fileName());
        return;
    }
#endif

    // All strategies failed
    QMessageBox::warning(
        this,
        "Editor Error",
        QString("Failed to open file in editor.\n\n"
                "File: %1\n\n"
                "Please configure an editor in the External Editor setting below.")
            .arg(path)
    );
}

bool DialogSettingsQt::launchEditor(const QString& editorCmd, const QString& filePath)
{
    if (editorCmd.isEmpty()) {
        return false;
    }

    QString cmd = editorCmd;

    // Quote the file path if it contains spaces
    QString quotedPath = filePath;
    if (quotedPath.contains(' ') && !quotedPath.startsWith('"')) {
        quotedPath = "\"" + quotedPath + "\"";
    }

    // Replace %f placeholder or append file path
    if (cmd.contains("%f")) {
        cmd.replace("%f", quotedPath);
    } else {
        cmd += " " + quotedPath;
    }

    // Parse command line into program and arguments
    QString program;
    QStringList args;
    bool inQuote = false;
    QString currentArg;

    for (int i = 0; i < cmd.length(); ++i) {
        QChar c = cmd[i];

        if (c == '"') {
            inQuote = !inQuote;
        } else if (c == ' ' && !inQuote) {
            if (!currentArg.isEmpty()) {
                if (program.isEmpty()) {
                    program = currentArg;
                } else {
                    args.append(currentArg);
                }
                currentArg.clear();
            }
        } else {
            currentArg += c;
        }
    }

    if (!currentArg.isEmpty()) {
        if (program.isEmpty()) {
            program = currentArg;
        } else {
            args.append(currentArg);
        }
    }

    if (program.isEmpty()) {
        return false;
    }

    QProcess* process = new QProcess(this);
    process->setProgram(program);
    process->setArguments(args);

    connect(process, QOverload<QProcess::ProcessError>::of(&QProcess::errorOccurred),
            process, &QProcess::deleteLater);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            process, &QProcess::deleteLater);

    qint64 pid = 0;
    if (process->startDetached(&pid)) {
        return true;
    }

    delete process;
    return false;
}

void DialogSettingsQt::onRemoveKeymap()
{
    QListWidgetItem* item = m_keymapList->currentItem();
    if (!item) {
        return;
    }

    QString file = item->text();

    int ret = QMessageBox::question(
        this,
        "Remove Keymap",
        "Remove keymap file from list?\n\n" + file,
        QMessageBox::Yes | QMessageBox::No
    );

    if (ret == QMessageBox::Yes) {
        m_keymapFiles.removeAll(file);
        delete m_keymapList->takeItem(m_keymapList->currentRow());
        m_labelStatus->setText("Removed: " + file);
    }
}

void DialogSettingsQt::onBrowseKeymap()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Keymap Directory",
        m_editKeymapPath->text().isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
            : m_editKeymapPath->text()
    );

    if (!dir.isEmpty()) {
        m_editKeymapPath->setText(dir);
    }
}

void DialogSettingsQt::onBrowseEditor()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        "Select Editor Executable",
        "/usr/bin",
        "Executables (*)"
    );

    if (!file.isEmpty()) {
        // Quote the path if it contains spaces and add %f placeholder
        if (file.contains(' ')) {
            file = "\"" + file + "\"";
        }
        m_editEditorCommand->setText(file + " %f");
    }
}

void DialogSettingsQt::onKeymapSelectionChanged()
{
    bool hasSelection = m_keymapList->currentItem() != nullptr;
    m_btnEdit->setEnabled(hasSelection);
    m_btnRemove->setEnabled(hasSelection);
}

void DialogSettingsQt::onSave()
{
    saveSettings();
    m_labelStatus->setText("Settings saved successfully");

    QMessageBox::information(
        this,
        "YAMY",
        "Settings saved. Reload configuration to apply changes."
    );

    accept();
}

void DialogSettingsQt::onCancel()
{
    reject();
}

void DialogSettingsQt::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Keymap files group
    QGroupBox* keymapGroup = new QGroupBox("Keymap Files");
    QVBoxLayout* keymapLayout = new QVBoxLayout(keymapGroup);

    m_keymapList = new QListWidget();
    m_keymapList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_keymapList, &QListWidget::itemSelectionChanged,
            this, &DialogSettingsQt::onKeymapSelectionChanged);
    keymapLayout->addWidget(m_keymapList);

    // Button row
    QHBoxLayout* btnLayout = new QHBoxLayout();

    m_btnAdd = new QPushButton("Add...");
    connect(m_btnAdd, &QPushButton::clicked, this, &DialogSettingsQt::onAddKeymap);
    btnLayout->addWidget(m_btnAdd);

    m_btnEdit = new QPushButton("Edit...");
    m_btnEdit->setEnabled(false);
    connect(m_btnEdit, &QPushButton::clicked, this, &DialogSettingsQt::onEditKeymap);
    btnLayout->addWidget(m_btnEdit);

    m_btnRemove = new QPushButton("Remove");
    m_btnRemove->setEnabled(false);
    connect(m_btnRemove, &QPushButton::clicked, this, &DialogSettingsQt::onRemoveKeymap);
    btnLayout->addWidget(m_btnRemove);

    btnLayout->addStretch();
    keymapLayout->addLayout(btnLayout);
    mainLayout->addWidget(keymapGroup);

    // Keymap path group
    QGroupBox* pathGroup = new QGroupBox("Keymap Directory");
    QHBoxLayout* pathLayout = new QHBoxLayout(pathGroup);

    m_editKeymapPath = new QLineEdit();
    m_editKeymapPath->setPlaceholderText("Directory containing .mayu files");
    pathLayout->addWidget(m_editKeymapPath);

    m_btnBrowse = new QPushButton("Browse...");
    connect(m_btnBrowse, &QPushButton::clicked, this, &DialogSettingsQt::onBrowseKeymap);
    pathLayout->addWidget(m_btnBrowse);

    mainLayout->addWidget(pathGroup);

    // Editor configuration group
    QGroupBox* editorGroup = new QGroupBox("External Editor");
    QVBoxLayout* editorLayout = new QVBoxLayout(editorGroup);

    QHBoxLayout* editorCmdLayout = new QHBoxLayout();
    m_editEditorCommand = new QLineEdit();
    m_editEditorCommand->setPlaceholderText("Leave empty to use system default ($EDITOR or xdg-open)");
    editorCmdLayout->addWidget(m_editEditorCommand);

    m_btnBrowseEditor = new QPushButton("Browse...");
    connect(m_btnBrowseEditor, &QPushButton::clicked, this, &DialogSettingsQt::onBrowseEditor);
    editorCmdLayout->addWidget(m_btnBrowseEditor);

    editorLayout->addLayout(editorCmdLayout);

    QLabel* editorHelp = new QLabel(
        "Specify a command to open configuration files. Use %f as a placeholder for the file path.\n"
        "Examples: code %f, gedit %f, vim %f, nano %f"
    );
    editorHelp->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
    editorHelp->setWordWrap(true);
    editorLayout->addWidget(editorHelp);

    mainLayout->addWidget(editorGroup);

    // Status label
    m_labelStatus = new QLabel();
    m_labelStatus->setStyleSheet("QLabel { color: #666; }");
    mainLayout->addWidget(m_labelStatus);

    // Dialog buttons
    QHBoxLayout* dialogBtnLayout = new QHBoxLayout();
    dialogBtnLayout->addStretch();

    m_btnSave = new QPushButton("Save");
    m_btnSave->setDefault(true);
    connect(m_btnSave, &QPushButton::clicked, this, &DialogSettingsQt::onSave);
    dialogBtnLayout->addWidget(m_btnSave);

    m_btnCancel = new QPushButton("Cancel");
    connect(m_btnCancel, &QPushButton::clicked, this, &DialogSettingsQt::onCancel);
    dialogBtnLayout->addWidget(m_btnCancel);

    mainLayout->addLayout(dialogBtnLayout);
}

void DialogSettingsQt::loadSettings()
{
    QSettings settings("YAMY", "YAMY");

    // Load keymap files
    m_keymapFiles = settings.value("keymaps/files").toStringList();
    m_keymapList->clear();
    m_keymapList->addItems(m_keymapFiles);

    // Load keymap directory
    QString keymapDir = settings.value("keymaps/directory",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.yamy"
    ).toString();
    m_editKeymapPath->setText(keymapDir);

    // Load editor command
    QString editorCmd = settings.value("editor/command", "").toString();
    m_editEditorCommand->setText(editorCmd);

    m_labelStatus->setText("Settings loaded");
}

void DialogSettingsQt::saveSettings()
{
    QSettings settings("YAMY", "YAMY");

    // Save keymap files
    settings.setValue("keymaps/files", m_keymapFiles);

    // Save keymap directory
    settings.setValue("keymaps/directory", m_editKeymapPath->text());

    // Save editor command
    settings.setValue("editor/command", m_editEditorCommand->text());

    settings.sync();
}
