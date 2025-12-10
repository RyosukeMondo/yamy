#include "config_metadata_dialog.h"
#include "../../core/settings/config_metadata.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QDateTime>
#include <QFileInfo>

ConfigMetadataDialog::ConfigMetadataDialog(const QString& configPath, QWidget* parent)
    : QDialog(parent)
    , m_configPath(configPath)
    , m_saved(false)
    , m_editName(nullptr)
    , m_editDescription(nullptr)
    , m_editAuthor(nullptr)
    , m_editTags(nullptr)
    , m_labelCreated(nullptr)
    , m_labelModified(nullptr)
    , m_btnSave(nullptr)
    , m_btnCancel(nullptr)
    , m_labelValidation(nullptr)
{
    setWindowTitle("Edit Configuration Metadata");
    setMinimumSize(450, 400);

    setupUI();
    loadMetadata();
}

ConfigMetadataDialog::~ConfigMetadataDialog()
{
}

void ConfigMetadataDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Editable fields group
    QGroupBox* editGroup = new QGroupBox("Configuration Details");
    QFormLayout* formLayout = new QFormLayout(editGroup);

    m_editName = new QLineEdit();
    m_editName->setPlaceholderText("Enter display name...");
    m_editName->setToolTip("Display name for this configuration");
    connect(m_editName, &QLineEdit::textChanged, this, &ConfigMetadataDialog::validateInput);
    formLayout->addRow("Name:", m_editName);

    m_editDescription = new QTextEdit();
    m_editDescription->setPlaceholderText("Enter description...");
    m_editDescription->setToolTip("Description of what this configuration does");
    m_editDescription->setMaximumHeight(100);
    formLayout->addRow("Description:", m_editDescription);

    m_editAuthor = new QLineEdit();
    m_editAuthor->setPlaceholderText("Enter author name...");
    m_editAuthor->setToolTip("Author of this configuration");
    formLayout->addRow("Author:", m_editAuthor);

    m_editTags = new QLineEdit();
    m_editTags->setPlaceholderText("tag1, tag2, tag3...");
    m_editTags->setToolTip("Comma-separated list of tags for organization");
    formLayout->addRow("Tags:", m_editTags);

    mainLayout->addWidget(editGroup);

    // Read-only dates group
    QGroupBox* datesGroup = new QGroupBox("Timestamps");
    QFormLayout* datesLayout = new QFormLayout(datesGroup);

    m_labelCreated = new QLabel();
    m_labelCreated->setStyleSheet("QLabel { color: #666; }");
    datesLayout->addRow("Created:", m_labelCreated);

    m_labelModified = new QLabel();
    m_labelModified->setStyleSheet("QLabel { color: #666; }");
    datesLayout->addRow("Last Modified:", m_labelModified);

    mainLayout->addWidget(datesGroup);

    // Validation message
    m_labelValidation = new QLabel();
    m_labelValidation->setStyleSheet("QLabel { color: #cc0000; }");
    m_labelValidation->setWordWrap(true);
    m_labelValidation->hide();
    mainLayout->addWidget(m_labelValidation);

    mainLayout->addStretch();

    // Button row
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_btnCancel = new QPushButton("Cancel");
    connect(m_btnCancel, &QPushButton::clicked, this, &ConfigMetadataDialog::onCancel);
    btnLayout->addWidget(m_btnCancel);

    m_btnSave = new QPushButton("Save");
    m_btnSave->setDefault(true);
    connect(m_btnSave, &QPushButton::clicked, this, &ConfigMetadataDialog::onSave);
    btnLayout->addWidget(m_btnSave);

    mainLayout->addLayout(btnLayout);
}

void ConfigMetadataDialog::loadMetadata()
{
    ConfigMetadata metadata;
    metadata.load(m_configPath.toStdString());

    const ConfigMetadataInfo& info = metadata.info();

    // Populate fields
    m_editName->setText(QString::fromStdString(info.name));
    m_editDescription->setPlainText(QString::fromStdString(info.description));
    m_editAuthor->setText(QString::fromStdString(info.author));

    // Convert tags vector to comma-separated string
    QStringList tagList;
    for (const auto& tag : info.tags) {
        tagList << QString::fromStdString(tag);
    }
    m_editTags->setText(tagList.join(", "));

    // Display dates
    m_labelCreated->setText(formatTimestamp(info.createdDate));
    m_labelModified->setText(formatTimestamp(info.modifiedDate));

    // If name is empty, use filename as default
    if (m_editName->text().isEmpty()) {
        QFileInfo fileInfo(m_configPath);
        m_editName->setText(fileInfo.completeBaseName());
    }
}

void ConfigMetadataDialog::onSave()
{
    // Validate name
    QString name = m_editName->text().trimmed();
    if (name.isEmpty()) {
        m_labelValidation->setText("Name cannot be empty.");
        m_labelValidation->show();
        m_editName->setFocus();
        return;
    }

    // Load existing metadata to preserve createdDate
    ConfigMetadata metadata;
    metadata.load(m_configPath.toStdString());

    // Update fields
    metadata.setName(name.toStdString());
    metadata.setDescription(m_editDescription->toPlainText().toStdString());
    metadata.setAuthor(m_editAuthor->text().trimmed().toStdString());

    // Parse and set tags
    metadata.clearTags();
    QString tagsText = m_editTags->text().trimmed();
    if (!tagsText.isEmpty()) {
        QStringList tags = tagsText.split(',', Qt::SkipEmptyParts);
        for (const QString& tag : tags) {
            QString trimmedTag = tag.trimmed();
            if (!trimmedTag.isEmpty()) {
                metadata.addTag(trimmedTag.toStdString());
            }
        }
    }

    // Save metadata
    if (metadata.save(m_configPath.toStdString())) {
        m_saved = true;
        accept();
    } else {
        QMessageBox::critical(
            this,
            "Error",
            "Failed to save metadata.\n\n"
            "Please check file permissions and try again."
        );
    }
}

void ConfigMetadataDialog::onCancel()
{
    reject();
}

void ConfigMetadataDialog::validateInput()
{
    QString name = m_editName->text().trimmed();
    if (name.isEmpty()) {
        m_labelValidation->setText("Name cannot be empty.");
        m_labelValidation->show();
        m_btnSave->setEnabled(false);
    } else {
        m_labelValidation->hide();
        m_btnSave->setEnabled(true);
    }
}

QString ConfigMetadataDialog::formatTimestamp(time_t timestamp) const
{
    if (timestamp == 0) {
        return "Not set";
    }
    QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp);
    return dt.toString("yyyy-MM-dd HH:mm:ss");
}
