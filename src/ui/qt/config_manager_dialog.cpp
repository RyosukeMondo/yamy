#include "config_manager_dialog.h"
#include "config_metadata_dialog.h"
#include "../../core/settings/config_manager.h"
#include "../../core/settings/config_validator.h"
#include "../../core/settings/config_metadata.h"
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMutexLocker>
#include <QPainter>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QTextEdit>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>
#include <cstdlib>

ConfigManagerDialog::ConfigManagerDialog(QWidget* parent)
    : QDialog(parent)
    , m_searchEdit(nullptr)
    , m_btnClearSearch(nullptr)
    , m_statusFilter(nullptr)
    , m_configList(nullptr)
    , m_btnNew(nullptr)
    , m_btnDuplicate(nullptr)
    , m_btnDelete(nullptr)
    , m_btnRename(nullptr)
    , m_btnEdit(nullptr)
    , m_btnMetadata(nullptr)
    , m_btnImport(nullptr)
    , m_btnExport(nullptr)
    , m_btnSetActive(nullptr)
    , m_btnClose(nullptr)
    , m_labelStatus(nullptr)
    , m_labelPath(nullptr)
    , m_validationDetails(nullptr)
    , m_fileWatcher(nullptr)
    , m_validationDebounceTimer(nullptr)
    , m_validationThread(nullptr)
    , m_validationWorker(nullptr)
{
    setWindowTitle("Manage Configurations");
    setMinimumSize(700, 600);

    createValidationIcons();
    setupUI();
    setupFileWatcher();

    // Setup validation worker thread
    m_validationThread = new QThread(this);
    m_validationWorker = new ConfigValidationWorker();
    m_validationWorker->moveToThread(m_validationThread);

    connect(m_validationWorker, &ConfigValidationWorker::validationComplete,
            this, &ConfigManagerDialog::onValidationComplete);
    connect(this, &QObject::destroyed, m_validationThread, &QThread::quit);

    m_validationThread->start();

    // Setup debounce timer
    m_validationDebounceTimer = new QTimer(this);
    m_validationDebounceTimer->setSingleShot(true);
    m_validationDebounceTimer->setInterval(500);  // 500ms debounce
    connect(m_validationDebounceTimer, &QTimer::timeout, this, [this]() {
        for (const QString& path : m_pendingValidations) {
            startValidation(path);
        }
        m_pendingValidations.clear();
    });

    refreshConfigList();
}

ConfigManagerDialog::~ConfigManagerDialog()
{
    if (m_validationThread) {
        m_validationThread->quit();
        m_validationThread->wait();
    }
    delete m_validationWorker;
}

void ConfigManagerDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Create splitter for list and validation details
    QSplitter* splitter = new QSplitter(Qt::Vertical);

    // Configuration list group
    QGroupBox* listGroup = new QGroupBox("Available Configurations");
    QVBoxLayout* listLayout = new QVBoxLayout(listGroup);

    // Search and filter row
    QHBoxLayout* searchFilterLayout = new QHBoxLayout();

    // Search field
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search by name, description, or tags...");
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &ConfigManagerDialog::onSearchTextChanged);
    searchFilterLayout->addWidget(m_searchEdit, 1);

    // Status filter combo box
    m_statusFilter = new QComboBox();
    m_statusFilter->addItem("All", -1);
    m_statusFilter->addItem("Valid", 0);
    m_statusFilter->addItem("Has Warnings", 1);
    m_statusFilter->addItem("Has Errors", 2);
    m_statusFilter->setToolTip("Filter by validation status");
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigManagerDialog::onStatusFilterChanged);
    searchFilterLayout->addWidget(m_statusFilter);

    listLayout->addLayout(searchFilterLayout);

    m_configList = new QListWidget();
    m_configList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_configList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_configList, &QListWidget::itemSelectionChanged,
            this, &ConfigManagerDialog::onSelectionChanged);
    connect(m_configList, &QListWidget::itemDoubleClicked,
            this, &ConfigManagerDialog::onItemDoubleClicked);
    connect(m_configList, &QWidget::customContextMenuRequested,
            this, &ConfigManagerDialog::onContextMenuRequested);
    listLayout->addWidget(m_configList);

    // Path label below list
    m_labelPath = new QLabel();
    m_labelPath->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
    m_labelPath->setWordWrap(true);
    listLayout->addWidget(m_labelPath);

    splitter->addWidget(listGroup);

    // Validation details group
    QGroupBox* validationGroup = new QGroupBox("Validation Details");
    QVBoxLayout* validationLayout = new QVBoxLayout(validationGroup);

    m_validationDetails = new QTextEdit();
    m_validationDetails->setReadOnly(true);
    m_validationDetails->setFont(QFont("monospace", 9));
    m_validationDetails->setPlaceholderText("Select a configuration to see validation status...");
    validationLayout->addWidget(m_validationDetails);

    splitter->addWidget(validationGroup);
    splitter->setStretchFactor(0, 2);  // Config list gets 2/3
    splitter->setStretchFactor(1, 1);  // Validation details gets 1/3

    mainLayout->addWidget(splitter);

    // Button row 1: File operations
    QHBoxLayout* btnLayout1 = new QHBoxLayout();

    m_btnNew = new QPushButton("New...");
    m_btnNew->setToolTip("Create a new configuration from template");
    connect(m_btnNew, &QPushButton::clicked, this, &ConfigManagerDialog::onNew);
    btnLayout1->addWidget(m_btnNew);

    m_btnDuplicate = new QPushButton("Duplicate");
    m_btnDuplicate->setToolTip("Create a copy of the selected configuration");
    m_btnDuplicate->setEnabled(false);
    connect(m_btnDuplicate, &QPushButton::clicked, this, &ConfigManagerDialog::onDuplicate);
    btnLayout1->addWidget(m_btnDuplicate);

    m_btnDelete = new QPushButton("Delete");
    m_btnDelete->setToolTip("Delete the selected configuration");
    m_btnDelete->setEnabled(false);
    connect(m_btnDelete, &QPushButton::clicked, this, &ConfigManagerDialog::onDelete);
    btnLayout1->addWidget(m_btnDelete);

    m_btnRename = new QPushButton("Rename...");
    m_btnRename->setToolTip("Rename the selected configuration");
    m_btnRename->setEnabled(false);
    connect(m_btnRename, &QPushButton::clicked, this, &ConfigManagerDialog::onRename);
    btnLayout1->addWidget(m_btnRename);

    m_btnEdit = new QPushButton("Edit...");
    m_btnEdit->setToolTip("Open the configuration in an external editor");
    m_btnEdit->setEnabled(false);
    connect(m_btnEdit, &QPushButton::clicked, this, &ConfigManagerDialog::onEdit);
    btnLayout1->addWidget(m_btnEdit);

    m_btnMetadata = new QPushButton("Metadata...");
    m_btnMetadata->setToolTip("Edit configuration metadata (name, description, tags)");
    m_btnMetadata->setEnabled(false);
    connect(m_btnMetadata, &QPushButton::clicked, this, &ConfigManagerDialog::onEditMetadata);
    btnLayout1->addWidget(m_btnMetadata);

    btnLayout1->addStretch();
    mainLayout->addLayout(btnLayout1);

    // Button row 2: Import/Export and Set Active
    QHBoxLayout* btnLayout2 = new QHBoxLayout();

    m_btnImport = new QPushButton("Import...");
    m_btnImport->setToolTip("Import a configuration from an archive");
    connect(m_btnImport, &QPushButton::clicked, this, &ConfigManagerDialog::onImport);
    btnLayout2->addWidget(m_btnImport);

    m_btnExport = new QPushButton("Export...");
    m_btnExport->setToolTip("Export the selected configuration to an archive");
    m_btnExport->setEnabled(false);
    connect(m_btnExport, &QPushButton::clicked, this, &ConfigManagerDialog::onExport);
    btnLayout2->addWidget(m_btnExport);

    btnLayout2->addStretch();

    m_btnSetActive = new QPushButton("Set as Active");
    m_btnSetActive->setToolTip("Make this the currently active configuration");
    m_btnSetActive->setEnabled(false);
    connect(m_btnSetActive, &QPushButton::clicked, this, &ConfigManagerDialog::onSetActive);
    btnLayout2->addWidget(m_btnSetActive);

    mainLayout->addLayout(btnLayout2);

    // Status label
    m_labelStatus = new QLabel();
    m_labelStatus->setStyleSheet("QLabel { color: #666; }");
    mainLayout->addWidget(m_labelStatus);

    // Dialog buttons
    QHBoxLayout* dialogBtnLayout = new QHBoxLayout();
    dialogBtnLayout->addStretch();

    m_btnClose = new QPushButton("Close");
    m_btnClose->setDefault(true);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);
    dialogBtnLayout->addWidget(m_btnClose);

    mainLayout->addLayout(dialogBtnLayout);
}

// User roles for storing data in list items
static const int RoleConfigPath = Qt::UserRole;
static const int RoleConfigIndex = Qt::UserRole + 1;
static const int RoleSearchableName = Qt::UserRole + 2;
static const int RoleDescription = Qt::UserRole + 3;
static const int RoleTags = Qt::UserRole + 4;

void ConfigManagerDialog::refreshConfigList()
{
    m_configList->clear();

    // Clear file watcher and re-add files
    if (m_fileWatcher) {
        QStringList watched = m_fileWatcher->files();
        if (!watched.isEmpty()) {
            m_fileWatcher->removePaths(watched);
        }
    }

    ConfigManager& configMgr = ConfigManager::instance();
    std::vector<ConfigEntry> configs = configMgr.listConfigs();
    int activeIndex = configMgr.getActiveIndex();

    for (size_t i = 0; i < configs.size(); ++i) {
        const ConfigEntry& config = configs[i];
        QString baseName = QString::fromStdString(config.name);
        QString displayName = baseName;
        QString configPath = QString::fromStdString(config.path);

        // Add indicator for active config
        if (static_cast<int>(i) == activeIndex) {
            displayName = QString::fromUtf8("\u2713 ") + displayName + " (active)";
        }

        // Add indicator for missing files
        if (!config.exists) {
            displayName += " [missing]";
        }

        QListWidgetItem* item = new QListWidgetItem(displayName);
        item->setData(RoleConfigPath, configPath);
        item->setData(RoleConfigIndex, static_cast<int>(i));
        item->setData(RoleSearchableName, baseName);

        // Load metadata for search
        ConfigMetadata metadata;
        if (metadata.load(configPath.toStdString())) {
            const ConfigMetadataInfo& info = metadata.info();
            QString description = QString::fromStdString(info.description);
            QStringList tags;
            for (const auto& tag : info.tags) {
                tags << QString::fromStdString(tag);
            }
            item->setData(RoleDescription, description);
            item->setData(RoleTags, tags.join(" "));
        } else {
            item->setData(RoleDescription, QString());
            item->setData(RoleTags, QString());
        }

        // Set initial icon as pending
        item->setIcon(m_iconPending);

        // Style missing files differently
        if (!config.exists) {
            item->setForeground(Qt::red);
            item->setIcon(m_iconError);
        } else if (static_cast<int>(i) == activeIndex) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }

        m_configList->addItem(item);

        // Add to file watcher if file exists
        if (config.exists && m_fileWatcher) {
            m_fileWatcher->addPath(configPath);
        }
    }

    m_labelStatus->setText(QString("%1 configuration(s) found").arg(configs.size()));
    updateButtonStates();

    // Apply current filters
    applyFilters();

    // Start validation for all configs
    startValidationForAll();
}

void ConfigManagerDialog::onSelectionChanged()
{
    updateButtonStates();

    // Update path label
    QString path = getSelectedConfigPath();
    if (!path.isEmpty()) {
        m_labelPath->setText("Path: " + path);

        // Show validation details for selected config
        QMutexLocker locker(&m_validationCacheMutex);
        if (m_validationCache.contains(path)) {
            const ValidationStatus& status = m_validationCache[path];
            if (status.messages.isEmpty()) {
                m_validationDetails->setHtml(
                    "<span style='color: green;'>&#10004; Configuration is valid</span>");
            } else {
                QString html = "<style>"
                    "body { font-family: monospace; }"
                    ".error { color: #cc0000; }"
                    ".warning { color: #cc7700; }"
                    ".line { color: #666666; }"
                    "</style>";

                for (const QString& msg : status.messages) {
                    QString styleClass = msg.contains("error") ? "error" : "warning";
                    QString escapedMsg = msg.toHtmlEscaped().replace("\n", "<br>");
                    html += QString("<p class='%1'>%2</p>").arg(styleClass, escapedMsg);
                }
                m_validationDetails->setHtml(html);
            }
        } else {
            m_validationDetails->setHtml(
                "<span style='color: #666;'>Validating...</span>");
        }
    } else {
        m_labelPath->clear();
        m_validationDetails->clear();
    }
}

void ConfigManagerDialog::onItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) {
        return;
    }

    QString path = item->data(Qt::UserRole).toString();
    if (!path.isEmpty()) {
        openInEditor(path);
    }
}

void ConfigManagerDialog::updateButtonStates()
{
    bool hasSelection = m_configList->currentItem() != nullptr;
    QString path = getSelectedConfigPath();
    bool fileExists = hasSelection && QFileInfo::exists(path);

    ConfigManager& configMgr = ConfigManager::instance();
    int activeIndex = configMgr.getActiveIndex();
    int selectedIndex = getSelectedIndex();
    bool isActive = (selectedIndex >= 0 && selectedIndex == activeIndex);

    m_btnDuplicate->setEnabled(fileExists);
    m_btnDelete->setEnabled(hasSelection && !isActive);  // Can't delete active config
    m_btnRename->setEnabled(fileExists && !isActive);    // Can't rename active config
    m_btnEdit->setEnabled(fileExists);
    m_btnMetadata->setEnabled(fileExists);
    m_btnExport->setEnabled(fileExists);
    m_btnSetActive->setEnabled(fileExists && !isActive);
}

QString ConfigManagerDialog::getSelectedConfigPath() const
{
    QListWidgetItem* item = m_configList->currentItem();
    if (!item) {
        return QString();
    }
    return item->data(Qt::UserRole).toString();
}

int ConfigManagerDialog::getSelectedIndex() const
{
    QListWidgetItem* item = m_configList->currentItem();
    if (!item) {
        return -1;
    }
    return item->data(Qt::UserRole + 1).toInt();
}

bool ConfigManagerDialog::validateConfigName(const QString& name) const
{
    if (name.isEmpty()) {
        return false;
    }

    // Check for invalid characters
    static const QString invalidChars = "/\\:*?\"<>|";
    for (QChar c : name) {
        if (invalidChars.contains(c)) {
            return false;
        }
    }

    return true;
}

void ConfigManagerDialog::openInEditor(const QString& path)
{
    QSettings settings("YAMY", "YAMY");
    QString editorCmd = settings.value("editor/command", "").toString();

    // Strategy 1: Use configured editor command
    if (!editorCmd.isEmpty()) {
        if (launchEditor(editorCmd, path)) {
            return;
        }
        // Fall through to other strategies if configured editor fails
    }

    // Strategy 2: Try $EDITOR environment variable (Linux/Unix convention)
    const char* envEditor = std::getenv("EDITOR");
    if (envEditor && strlen(envEditor) > 0) {
        QString envEditorCmd = QString::fromLocal8Bit(envEditor) + " %f";
        if (launchEditor(envEditorCmd, path)) {
            return;
        }
    }

    // Strategy 3: Try $VISUAL environment variable (for GUI editors)
    const char* envVisual = std::getenv("VISUAL");
    if (envVisual && strlen(envVisual) > 0) {
        QString visualCmd = QString::fromLocal8Bit(envVisual) + " %f";
        if (launchEditor(visualCmd, path)) {
            return;
        }
    }

    // Strategy 4: Use QDesktopServices (system default)
    QUrl fileUrl = QUrl::fromLocalFile(path);
    if (QDesktopServices::openUrl(fileUrl)) {
        return;
    }

    // Strategy 5: Fallback to xdg-open on Linux
#ifdef Q_OS_LINUX
    QProcess process;
    process.setProgram("xdg-open");
    process.setArguments({path});
    if (process.startDetached()) {
        return;
    }
#endif

#ifdef Q_OS_WIN
    // Strategy 5 (Windows): Fallback to notepad
    QProcess process;
    process.setProgram("notepad.exe");
    process.setArguments({path});
    if (process.startDetached()) {
        return;
    }
#endif

    // All strategies failed - show error
    QMessageBox::warning(
        this,
        "Editor Error",
        QString("Failed to open configuration file in editor.\n\n"
                "File: %1\n\n"
                "Please configure an editor in Settings or set the $EDITOR environment variable.")
            .arg(path)
    );
}

bool ConfigManagerDialog::launchEditor(const QString& editorCmd, const QString& filePath)
{
    if (editorCmd.isEmpty()) {
        return false;
    }

    // Parse the editor command
    // Support both "editor %f" and just "editor" formats
    QString cmd = editorCmd;
    QStringList args;

    // Quote the file path if it contains spaces
    QString quotedPath = filePath;
    if (quotedPath.contains(' ') && !quotedPath.startsWith('"')) {
        quotedPath = "\"" + quotedPath + "\"";
    }

    // Replace %f placeholder with the file path
    if (cmd.contains("%f")) {
        cmd.replace("%f", quotedPath);
    } else {
        // No placeholder - append file path
        cmd += " " + quotedPath;
    }

    // Parse the command line into program and arguments
    // Handle quoted strings properly
    QString program;
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

    // Add the last argument
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

    // Launch the editor
    QProcess* process = new QProcess(this);
    process->setProgram(program);
    process->setArguments(args);

    // Connect to error signal for cleanup
    connect(process, QOverload<QProcess::ProcessError>::of(&QProcess::errorOccurred),
            this, [this, process, program](QProcess::ProcessError error) {
                Q_UNUSED(error);
                // Clean up process object
                process->deleteLater();
            });

    // Connect to finished signal for cleanup
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            process, &QProcess::deleteLater);

    // Start detached so the editor runs independently
    qint64 pid = 0;
    if (process->startDetached(&pid)) {
        return true;
    }

    // startDetached failed, clean up
    delete process;
    return false;
}

void ConfigManagerDialog::onNew()
{
    // Show template selection dialog
    QStringList templates;
    std::vector<std::string> templateList = ConfigManager::listTemplates();
    for (const auto& t : templateList) {
        templates << QString::fromStdString(t);
    }

    bool ok;
    QString templateName = QInputDialog::getItem(
        this,
        "New Configuration",
        "Select a template:",
        templates,
        0,
        false,
        &ok
    );

    if (!ok || templateName.isEmpty()) {
        return;
    }

    // Get name for new configuration
    QString configName = QInputDialog::getText(
        this,
        "New Configuration",
        "Configuration name:",
        QLineEdit::Normal,
        "my_config",
        &ok
    );

    if (!ok || configName.isEmpty()) {
        return;
    }

    if (!validateConfigName(configName)) {
        QMessageBox::warning(
            this,
            "Invalid Name",
            "Configuration name contains invalid characters.\n"
            "Avoid: / \\ : * ? \" < > |"
        );
        return;
    }

    // Create path for new config
    QString configDir = QString::fromStdString(ConfigManager::getDefaultConfigDir());
    QString targetPath = configDir + "/" + configName + ".mayu";

    // Check if file already exists
    if (QFileInfo::exists(targetPath)) {
        QMessageBox::warning(
            this,
            "File Exists",
            QString("A configuration with this name already exists:\n%1").arg(targetPath)
        );
        return;
    }

    // Create from template
    ConfigManager& configMgr = ConfigManager::instance();
    auto result = configMgr.createFromTemplate(
        templateName.toStdString(),
        targetPath.toStdString()
    );

    if (result.success) {
        // Add to config manager and refresh
        configMgr.addConfig(targetPath.toStdString());
        refreshConfigList();
        m_labelStatus->setText(QString("Created: %1").arg(configName));
    } else {
        QMessageBox::critical(
            this,
            "Error",
            QString("Failed to create configuration:\n%1")
                .arg(QString::fromStdString(result.errorMessage))
        );
    }
}

void ConfigManagerDialog::onDuplicate()
{
    QString sourcePath = getSelectedConfigPath();
    if (sourcePath.isEmpty()) {
        return;
    }

    QFileInfo sourceInfo(sourcePath);
    QString baseName = sourceInfo.completeBaseName();

    bool ok;
    QString newName = QInputDialog::getText(
        this,
        "Duplicate Configuration",
        "New configuration name:",
        QLineEdit::Normal,
        baseName + "_copy",
        &ok
    );

    if (!ok || newName.isEmpty()) {
        return;
    }

    if (!validateConfigName(newName)) {
        QMessageBox::warning(
            this,
            "Invalid Name",
            "Configuration name contains invalid characters.\n"
            "Avoid: / \\ : * ? \" < > |"
        );
        return;
    }

    QString targetPath = sourceInfo.dir().filePath(newName + ".mayu");

    if (QFileInfo::exists(targetPath)) {
        QMessageBox::warning(
            this,
            "File Exists",
            QString("A configuration with this name already exists:\n%1").arg(targetPath)
        );
        return;
    }

    // Copy the file
    if (QFile::copy(sourcePath, targetPath)) {
        ConfigManager& configMgr = ConfigManager::instance();
        configMgr.addConfig(targetPath.toStdString());
        refreshConfigList();
        m_labelStatus->setText(QString("Duplicated to: %1").arg(newName));
    } else {
        QMessageBox::critical(
            this,
            "Error",
            "Failed to duplicate configuration file."
        );
    }
}

void ConfigManagerDialog::onDelete()
{
    QString path = getSelectedConfigPath();
    if (path.isEmpty()) {
        return;
    }

    QFileInfo info(path);
    QString name = info.completeBaseName();

    int ret = QMessageBox::warning(
        this,
        "Delete Configuration",
        QString("Are you sure you want to delete '%1'?\n\n"
                "File: %2\n\n"
                "This action cannot be undone.")
            .arg(name)
            .arg(path),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (ret != QMessageBox::Yes) {
        return;
    }

    // Remove from config manager first
    ConfigManager& configMgr = ConfigManager::instance();
    configMgr.removeConfig(path.toStdString());

    // Delete the file if it exists
    if (QFile::exists(path)) {
        if (!QFile::remove(path)) {
            QMessageBox::warning(
                this,
                "Warning",
                "Configuration was removed from list but the file could not be deleted.\n"
                "You may need to delete it manually."
            );
        }
    }

    refreshConfigList();
    m_labelStatus->setText(QString("Deleted: %1").arg(name));
}

void ConfigManagerDialog::onRename()
{
    QString path = getSelectedConfigPath();
    if (path.isEmpty()) {
        return;
    }

    QFileInfo info(path);
    QString currentName = info.completeBaseName();

    bool ok;
    QString newName = QInputDialog::getText(
        this,
        "Rename Configuration",
        "New name:",
        QLineEdit::Normal,
        currentName,
        &ok
    );

    if (!ok || newName.isEmpty() || newName == currentName) {
        return;
    }

    if (!validateConfigName(newName)) {
        QMessageBox::warning(
            this,
            "Invalid Name",
            "Configuration name contains invalid characters.\n"
            "Avoid: / \\ : * ? \" < > |"
        );
        return;
    }

    QString newPath = info.dir().filePath(newName + ".mayu");

    if (QFileInfo::exists(newPath)) {
        QMessageBox::warning(
            this,
            "File Exists",
            QString("A configuration with this name already exists:\n%1").arg(newPath)
        );
        return;
    }

    // Rename the file
    if (QFile::rename(path, newPath)) {
        ConfigManager& configMgr = ConfigManager::instance();
        configMgr.removeConfig(path.toStdString());
        configMgr.addConfig(newPath.toStdString());
        refreshConfigList();
        m_labelStatus->setText(QString("Renamed to: %1").arg(newName));
    } else {
        QMessageBox::critical(
            this,
            "Error",
            "Failed to rename configuration file."
        );
    }
}

void ConfigManagerDialog::onEdit()
{
    QString path = getSelectedConfigPath();
    if (!path.isEmpty()) {
        openInEditor(path);
        m_labelStatus->setText("Opening in editor...");
    }
}

void ConfigManagerDialog::onImport()
{
    QString archivePath = QFileDialog::getOpenFileName(
        this,
        "Import Configuration",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        QString("YAMY Archives (*%1);;All Files (*)")
            .arg(QString::fromStdString(ConfigManager::ARCHIVE_EXTENSION))
    );

    if (archivePath.isEmpty()) {
        return;
    }

    // Get target directory
    QString targetDir = QString::fromStdString(ConfigManager::getDefaultConfigDir());

    ConfigManager& configMgr = ConfigManager::instance();
    auto result = configMgr.importConfig(
        archivePath.toStdString(),
        targetDir.toStdString(),
        false  // Don't overwrite without asking
    );

    if (result.success) {
        // Add imported configs to the manager
        for (const auto& file : result.filesProcessed) {
            if (file.find(".mayu") != std::string::npos) {
                configMgr.addConfig(file);
            }
        }
        refreshConfigList();
        m_labelStatus->setText(QString("Imported %1 file(s)").arg(result.filesProcessed.size()));
    } else {
        QMessageBox::critical(
            this,
            "Import Error",
            QString("Failed to import configuration:\n%1")
                .arg(QString::fromStdString(result.errorMessage))
        );
    }
}

void ConfigManagerDialog::onExport()
{
    QString sourcePath = getSelectedConfigPath();
    if (sourcePath.isEmpty()) {
        return;
    }

    QFileInfo info(sourcePath);
    QString defaultName = info.completeBaseName() +
        QString::fromStdString(ConfigManager::ARCHIVE_EXTENSION);

    QString exportDir = QString::fromStdString(ConfigManager::getExportDir());
    QDir().mkpath(exportDir);

    QString archivePath = QFileDialog::getSaveFileName(
        this,
        "Export Configuration",
        exportDir + "/" + defaultName,
        QString("YAMY Archives (*%1)")
            .arg(QString::fromStdString(ConfigManager::ARCHIVE_EXTENSION))
    );

    if (archivePath.isEmpty()) {
        return;
    }

    // Ensure correct extension
    if (!archivePath.endsWith(QString::fromStdString(ConfigManager::ARCHIVE_EXTENSION))) {
        archivePath += QString::fromStdString(ConfigManager::ARCHIVE_EXTENSION);
    }

    ConfigManager& configMgr = ConfigManager::instance();
    auto result = configMgr.exportConfig(
        sourcePath.toStdString(),
        archivePath.toStdString()
    );

    if (result.success) {
        m_labelStatus->setText(QString("Exported %1 file(s) to archive")
            .arg(result.filesProcessed.size()));

        QMessageBox::information(
            this,
            "Export Complete",
            QString("Configuration exported successfully to:\n%1\n\n"
                    "Included %2 file(s)")
                .arg(archivePath)
                .arg(result.filesProcessed.size())
        );
    } else {
        QMessageBox::critical(
            this,
            "Export Error",
            QString("Failed to export configuration:\n%1")
                .arg(QString::fromStdString(result.errorMessage))
        );
    }
}

void ConfigManagerDialog::onSetActive()
{
    int index = getSelectedIndex();
    if (index < 0) {
        return;
    }

    ConfigManager& configMgr = ConfigManager::instance();
    if (configMgr.setActiveConfig(index)) {
        std::vector<ConfigEntry> configs = configMgr.listConfigs();
        if (index < static_cast<int>(configs.size())) {
            QString name = QString::fromStdString(configs[index].name);
            m_labelStatus->setText(QString("Active configuration: %1").arg(name));
        }
        refreshConfigList();
    } else {
        QMessageBox::warning(
            this,
            "Error",
            "Failed to set active configuration."
        );
    }
}

void ConfigManagerDialog::createValidationIcons()
{
    const int iconSize = 16;

    // Valid icon (green checkmark)
    {
        QPixmap pixmap(iconSize, iconSize);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(0, 150, 0), 2));
        painter.drawLine(3, 8, 6, 12);
        painter.drawLine(6, 12, 13, 4);
        m_iconValid = QIcon(pixmap);
    }

    // Error icon (red X)
    {
        QPixmap pixmap(iconSize, iconSize);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(200, 0, 0), 2));
        painter.drawLine(3, 3, 13, 13);
        painter.drawLine(13, 3, 3, 13);
        m_iconError = QIcon(pixmap);
    }

    // Warning icon (yellow triangle with !)
    {
        QPixmap pixmap(iconSize, iconSize);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(200, 150, 0), 2));
        QPolygon triangle;
        triangle << QPoint(8, 2) << QPoint(2, 14) << QPoint(14, 14);
        painter.drawPolygon(triangle);
        painter.drawLine(8, 6, 8, 10);
        painter.drawPoint(8, 12);
        m_iconWarning = QIcon(pixmap);
    }

    // Pending icon (gray circle)
    {
        QPixmap pixmap(iconSize, iconSize);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(150, 150, 150), 2));
        painter.drawEllipse(2, 2, 12, 12);
        m_iconPending = QIcon(pixmap);
    }
}

QIcon ConfigManagerDialog::getValidationIcon(bool hasErrors, bool hasWarnings) const
{
    if (hasErrors) {
        return m_iconError;
    }
    if (hasWarnings) {
        return m_iconWarning;
    }
    return m_iconValid;
}

void ConfigManagerDialog::setupFileWatcher()
{
    m_fileWatcher = new QFileSystemWatcher(this);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
            this, &ConfigManagerDialog::onFileChanged);
}

void ConfigManagerDialog::onFileChanged(const QString& path)
{
    // Debounce file changes to avoid excessive validation
    m_pendingValidations.insert(path);
    m_validationDebounceTimer->start();

    // Re-add path to watcher (some systems remove it on change)
    if (!m_fileWatcher->files().contains(path) && QFileInfo::exists(path)) {
        m_fileWatcher->addPath(path);
    }
}

void ConfigManagerDialog::startValidation(const QString& configPath)
{
    if (configPath.isEmpty() || !QFileInfo::exists(configPath)) {
        return;
    }

    // Invoke validation in worker thread
    QMetaObject::invokeMethod(m_validationWorker, "validateConfig",
                              Qt::QueuedConnection,
                              Q_ARG(QString, configPath));
}

void ConfigManagerDialog::startValidationForAll()
{
    ConfigManager& configMgr = ConfigManager::instance();
    std::vector<ConfigEntry> configs = configMgr.listConfigs();

    for (const auto& config : configs) {
        if (config.exists) {
            startValidation(QString::fromStdString(config.path));
        }
    }
}

void ConfigManagerDialog::onValidationComplete(const QString& configPath,
                                               bool hasErrors,
                                               bool hasWarnings,
                                               const QStringList& errorMessages)
{
    // Update cache
    {
        QMutexLocker locker(&m_validationCacheMutex);
        ValidationStatus& status = m_validationCache[configPath];
        status.hasErrors = hasErrors;
        status.hasWarnings = hasWarnings;
        status.messages = errorMessages;
    }

    // Update list item icon
    updateItemValidationStatus(configPath, hasErrors, hasWarnings);

    // Re-apply filters since status may have changed
    applyFilters();

    // If this is the currently selected config, update the details view
    QString selectedPath = getSelectedConfigPath();
    if (selectedPath == configPath) {
        onSelectionChanged();  // Refresh the details view
    }
}

void ConfigManagerDialog::updateItemValidationStatus(const QString& configPath,
                                                     bool hasErrors,
                                                     bool hasWarnings)
{
    // Find the list item with this path
    for (int i = 0; i < m_configList->count(); ++i) {
        QListWidgetItem* item = m_configList->item(i);
        if (item && item->data(Qt::UserRole).toString() == configPath) {
            item->setIcon(getValidationIcon(hasErrors, hasWarnings));

            // Update tooltip
            if (hasErrors) {
                item->setToolTip("Configuration has errors");
            } else if (hasWarnings) {
                item->setToolTip("Configuration has warnings");
            } else {
                item->setToolTip("Configuration is valid");
            }
            break;
        }
    }
}

void ConfigManagerDialog::onContextMenuRequested(const QPoint& pos)
{
    QListWidgetItem* item = m_configList->itemAt(pos);
    if (!item) {
        return;
    }

    QString path = item->data(Qt::UserRole).toString();
    bool fileExists = QFileInfo::exists(path);

    QMenu contextMenu(this);

    // Edit metadata action
    QAction* metadataAction = contextMenu.addAction("Edit Metadata...");
    metadataAction->setEnabled(fileExists);
    connect(metadataAction, &QAction::triggered, this, &ConfigManagerDialog::onEditMetadata);

    contextMenu.addSeparator();

    // Edit in editor action
    QAction* editAction = contextMenu.addAction("Edit in Editor...");
    editAction->setEnabled(fileExists);
    connect(editAction, &QAction::triggered, this, &ConfigManagerDialog::onEdit);

    // Duplicate action
    QAction* duplicateAction = contextMenu.addAction("Duplicate");
    duplicateAction->setEnabled(fileExists);
    connect(duplicateAction, &QAction::triggered, this, &ConfigManagerDialog::onDuplicate);

    // Rename action
    ConfigManager& configMgr = ConfigManager::instance();
    int activeIndex = configMgr.getActiveIndex();
    int selectedIndex = item->data(Qt::UserRole + 1).toInt();
    bool isActive = (selectedIndex == activeIndex);

    QAction* renameAction = contextMenu.addAction("Rename...");
    renameAction->setEnabled(fileExists && !isActive);
    connect(renameAction, &QAction::triggered, this, &ConfigManagerDialog::onRename);

    contextMenu.addSeparator();

    // Export action
    QAction* exportAction = contextMenu.addAction("Export...");
    exportAction->setEnabled(fileExists);
    connect(exportAction, &QAction::triggered, this, &ConfigManagerDialog::onExport);

    contextMenu.addSeparator();

    // Set active action
    QAction* setActiveAction = contextMenu.addAction("Set as Active");
    setActiveAction->setEnabled(fileExists && !isActive);
    connect(setActiveAction, &QAction::triggered, this, &ConfigManagerDialog::onSetActive);

    contextMenu.addSeparator();

    // Delete action
    QAction* deleteAction = contextMenu.addAction("Delete");
    deleteAction->setEnabled(!isActive);
    connect(deleteAction, &QAction::triggered, this, &ConfigManagerDialog::onDelete);

    contextMenu.exec(m_configList->mapToGlobal(pos));
}

void ConfigManagerDialog::onEditMetadata()
{
    QString path = getSelectedConfigPath();
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return;
    }

    // Load the existing metadata
    ConfigMetadata metadata;
    metadata.load(path.toStdString());

    // Create and configure the dialog
    ConfigMetadataDialog dialog(this);
    dialog.setMetadata(metadata.info());

    // Show the dialog and check the result
    if (dialog.exec() == QDialog::Accepted) {
        // Get the updated metadata and save it
        ConfigMetadataInfo updatedInfo = dialog.getMetadata();
        metadata.info() = updatedInfo;
        
        if (metadata.save(path.toStdString())) {
            // Refresh the list to reflect changes
            refreshConfigList();
            m_labelStatus->setText("Metadata updated successfully");
        } else {
            QMessageBox::critical(
                this,
                "Error",
                "Failed to save metadata.\n\nPlease check file permissions and try again."
            );
        }
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Search and filter implementation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void ConfigManagerDialog::onSearchTextChanged(const QString& text)
{
    Q_UNUSED(text);
    applyFilters();
}

void ConfigManagerDialog::onStatusFilterChanged(int index)
{
    Q_UNUSED(index);
    applyFilters();
}

void ConfigManagerDialog::onClearSearch()
{
    m_searchEdit->clear();
}

bool ConfigManagerDialog::matchesFilter(const QString& configPath, const QString& displayName) const
{
    // Get search text (case-insensitive)
    QString searchText = m_searchEdit->text().trimmed().toLower();

    // Get status filter
    int statusFilter = m_statusFilter->currentData().toInt();

    // Check status filter first (more efficient to fail fast)
    if (statusFilter >= 0) {
        // Use const_cast to access mutex in const method - safe because we're only reading
        QMutexLocker locker(const_cast<QMutex*>(&m_validationCacheMutex));
        if (m_validationCache.contains(configPath)) {
            const ValidationStatus& status = m_validationCache[configPath];
            bool matches = false;
            switch (statusFilter) {
                case 0: // Valid - no errors and no warnings
                    matches = !status.hasErrors && !status.hasWarnings;
                    break;
                case 1: // Has Warnings
                    matches = status.hasWarnings && !status.hasErrors;
                    break;
                case 2: // Has Errors
                    matches = status.hasErrors;
                    break;
            }
            if (!matches) {
                return false;
            }
        } else {
            // Validation not yet complete - hide if filtering by status
            return false;
        }
    }

    // If no search text, match all (that passed status filter)
    if (searchText.isEmpty()) {
        return true;
    }

    // Search in name (from display name, minus the indicators)
    if (displayName.toLower().contains(searchText)) {
        return true;
    }

    // Search in metadata fields using stored data
    // We need to get the item to access the metadata
    for (int i = 0; i < m_configList->count(); ++i) {
        QListWidgetItem* item = m_configList->item(i);
        if (item && item->data(RoleConfigPath).toString() == configPath) {
            // Search in searchable name
            QString name = item->data(RoleSearchableName).toString().toLower();
            if (name.contains(searchText)) {
                return true;
            }
            // Search in description
            QString desc = item->data(RoleDescription).toString().toLower();
            if (desc.contains(searchText)) {
                return true;
            }
            // Search in tags
            QString tags = item->data(RoleTags).toString().toLower();
            if (tags.contains(searchText)) {
                return true;
            }
            break;
        }
    }

    return false;
}

void ConfigManagerDialog::applyFilters()
{
    int visibleCount = 0;
    int totalCount = m_configList->count();

    for (int i = 0; i < totalCount; ++i) {
        QListWidgetItem* item = m_configList->item(i);
        if (!item) continue;

        QString configPath = item->data(RoleConfigPath).toString();
        QString displayName = item->text();

        bool visible = matchesFilter(configPath, displayName);
        item->setHidden(!visible);

        if (visible) {
            ++visibleCount;
        }
    }

    // Update status label to show filter results
    if (visibleCount == totalCount) {
        m_labelStatus->setText(QString("%1 configuration(s) found").arg(totalCount));
    } else {
        m_labelStatus->setText(QString("Showing %1 of %2 configuration(s)")
            .arg(visibleCount).arg(totalCount));
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ConfigValidationWorker implementation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ConfigValidationWorker::ConfigValidationWorker(QObject* parent)
    : QObject(parent)
{
}

void ConfigValidationWorker::validateConfig(const QString& configPath)
{
    ConfigValidator validator;
    ValidationResult result = validator.validate(configPath.toStdString());

    QStringList errorMessages;
    for (const auto& error : result.errors) {
        QString msg;
        if (error.lineNumber > 0) {
            msg = QString("Line %1: ").arg(error.lineNumber);
        }
        msg += QString::fromStdString(error.message);
        if (!error.context.empty()) {
            msg += "\n  " + QString::fromStdString(error.context);
        }
        // Prefix with severity
        if (error.isError()) {
            msg = "[error] " + msg;
        } else {
            msg = "[warning] " + msg;
        }
        errorMessages.append(msg);
    }

    emit validationComplete(configPath, result.hasErrors, result.hasWarnings,
                           errorMessages);
}
