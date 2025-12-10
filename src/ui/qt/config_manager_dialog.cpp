#include "config_manager_dialog.h"
#include "../../core/settings/config_manager.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QGroupBox>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QProcess>

ConfigManagerDialog::ConfigManagerDialog(QWidget* parent)
    : QDialog(parent)
    , m_configList(nullptr)
    , m_btnNew(nullptr)
    , m_btnDuplicate(nullptr)
    , m_btnDelete(nullptr)
    , m_btnRename(nullptr)
    , m_btnEdit(nullptr)
    , m_btnImport(nullptr)
    , m_btnExport(nullptr)
    , m_btnSetActive(nullptr)
    , m_btnClose(nullptr)
    , m_labelStatus(nullptr)
    , m_labelPath(nullptr)
{
    setWindowTitle("Manage Configurations");
    setMinimumSize(700, 500);

    setupUI();
    refreshConfigList();
}

ConfigManagerDialog::~ConfigManagerDialog()
{
}

void ConfigManagerDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Configuration list group
    QGroupBox* listGroup = new QGroupBox("Available Configurations");
    QVBoxLayout* listLayout = new QVBoxLayout(listGroup);

    m_configList = new QListWidget();
    m_configList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_configList, &QListWidget::itemSelectionChanged,
            this, &ConfigManagerDialog::onSelectionChanged);
    connect(m_configList, &QListWidget::itemDoubleClicked,
            this, &ConfigManagerDialog::onItemDoubleClicked);
    listLayout->addWidget(m_configList);

    // Path label below list
    m_labelPath = new QLabel();
    m_labelPath->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
    m_labelPath->setWordWrap(true);
    listLayout->addWidget(m_labelPath);

    mainLayout->addWidget(listGroup);

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

void ConfigManagerDialog::refreshConfigList()
{
    m_configList->clear();

    ConfigManager& configMgr = ConfigManager::instance();
    std::vector<ConfigEntry> configs = configMgr.listConfigs();
    int activeIndex = configMgr.getActiveIndex();

    for (size_t i = 0; i < configs.size(); ++i) {
        const ConfigEntry& config = configs[i];
        QString displayName = QString::fromStdString(config.name);

        // Add indicator for active config
        if (static_cast<int>(i) == activeIndex) {
            displayName = QString::fromUtf8("\u2713 ") + displayName + " (active)";
        }

        // Add indicator for missing files
        if (!config.exists) {
            displayName += " [missing]";
        }

        QListWidgetItem* item = new QListWidgetItem(displayName);
        item->setData(Qt::UserRole, QString::fromStdString(config.path));
        item->setData(Qt::UserRole + 1, static_cast<int>(i));

        // Style missing files differently
        if (!config.exists) {
            item->setForeground(Qt::red);
        } else if (static_cast<int>(i) == activeIndex) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }

        m_configList->addItem(item);
    }

    m_labelStatus->setText(QString("%1 configuration(s) found").arg(configs.size()));
    updateButtonStates();
}

void ConfigManagerDialog::onSelectionChanged()
{
    updateButtonStates();

    // Update path label
    QString path = getSelectedConfigPath();
    if (!path.isEmpty()) {
        m_labelPath->setText("Path: " + path);
    } else {
        m_labelPath->clear();
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
    // Try to use the system default application
    QUrl fileUrl = QUrl::fromLocalFile(path);
    if (!QDesktopServices::openUrl(fileUrl)) {
        // Fallback: try xdg-open
        QProcess::startDetached("xdg-open", {path});
    }
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
