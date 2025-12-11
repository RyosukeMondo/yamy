#include "config_metadata_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QStringList>

ConfigMetadataDialog::ConfigMetadataDialog(QWidget* parent)
    : QDialog(parent)
    , m_editName(nullptr)
    , m_editDescription(nullptr)
    , m_editAuthor(nullptr)
    , m_editTags(nullptr)
    , m_btnSave(nullptr)
    , m_btnCancel(nullptr)
    , m_labelValidation(nullptr)
{
    setWindowTitle("Edit Configuration Metadata");
    setMinimumSize(450, 320);
    setupUI();
    validateInput();
}

ConfigMetadataDialog::~ConfigMetadataDialog()
{
}

void ConfigMetadataDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

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

    m_labelValidation = new QLabel();
    m_labelValidation->setStyleSheet("QLabel { color: #cc0000; }");
    m_labelValidation->setWordWrap(true);
    m_labelValidation->hide();
    mainLayout->addWidget(m_labelValidation);

    mainLayout->addStretch();

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

void ConfigMetadataDialog::setMetadata(const ConfigMetadataInfo& info)
{
    m_info = info; // Cache the original metadata

    m_editName->setText(QString::fromStdString(info.name));
    m_editDescription->setPlainText(QString::fromStdString(info.description));
    m_editAuthor->setText(QString::fromStdString(info.author));

    QStringList tagList;
    for (const auto& tag : info.tags) {
        tagList << QString::fromStdString(tag);
    }
    m_editTags->setText(tagList.join(", "));
    validateInput();
}

ConfigMetadataInfo ConfigMetadataDialog::getMetadata() const
{
    ConfigMetadataInfo updatedInfo = m_info; // Start with a copy of the original
    updatedInfo.name = m_editName->text().trimmed().toStdString();
    updatedInfo.description = m_editDescription->toPlainText().toStdString();
    updatedInfo.author = m_editAuthor->text().trimmed().toStdString();

    updatedInfo.tags.clear();
    QString tagsText = m_editTags->text().trimmed();
    if (!tagsText.isEmpty()) {
        QStringList tags = tagsText.split(',', Qt::SkipEmptyParts);
        for (const QString& tag : tags) {
            QString trimmedTag = tag.trimmed();
            if (!trimmedTag.isEmpty()) {
                updatedInfo.tags.push_back(trimmedTag.toStdString());
            }
        }
    }
    return updatedInfo;
}

void ConfigMetadataDialog::onSave()
{
    validateInput();
    if (m_btnSave->isEnabled()) {
        accept();
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
