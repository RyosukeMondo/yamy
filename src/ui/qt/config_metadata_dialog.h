#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QString>

#include "../../core/settings/config_metadata.h"

/**
 * @brief Dialog for editing configuration metadata
 *
 * Allows users to view and edit metadata fields for a configuration,
 * such as its name, description, author, and tags.
 *
 * The dialog is decoupled from file I/O. The parent component is
 * responsible for creating a ConfigMetadataInfo object, passing it to
 * the dialog via setMetadata(), and then retrieving the updated object
 * via getMetadata() if the dialog is accepted.
 */
class ConfigMetadataDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct metadata editor dialog
     * @param parent Parent widget
     */
    explicit ConfigMetadataDialog(QWidget* parent = nullptr);
    ~ConfigMetadataDialog() override;

    /**
     * @brief Set the metadata to be displayed and edited
     * @param info The metadata to populate the dialog with
     */
    void setMetadata(const ConfigMetadataInfo& info);

    /**
     * @brief Get the updated metadata from the dialog
     * @return An object containing the new metadata values
     */
    ConfigMetadataInfo getMetadata() const;

private slots:
    void onSave();
    void onCancel();
    void validateInput();

private:
    void setupUI();

    // UI Components
    QLineEdit* m_editName;
    QTextEdit* m_editDescription;
    QLineEdit* m_editAuthor;
    QLineEdit* m_editTags;
    QPushButton* m_btnSave;
    QPushButton* m_btnCancel;
    QLabel* m_labelValidation;

    // Cached metadata
    ConfigMetadataInfo m_info;
};
