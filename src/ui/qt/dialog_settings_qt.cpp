#include "dialog_settings_qt.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>

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

    // Open file in default editor
    // TODO: Use configured editor or system default
    QMessageBox::information(
        this,
        "Edit Keymap",
        "Edit functionality will open the file in your configured editor.\n\n"
        "File: " + file
    );
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

    m_labelStatus->setText("Settings loaded");
}

void DialogSettingsQt::saveSettings()
{
    QSettings settings("YAMY", "YAMY");

    // Save keymap files
    settings.setValue("keymaps/files", m_keymapFiles);

    // Save keymap directory
    settings.setValue("keymaps/directory", m_editKeymapPath->text());

    settings.sync();
}
