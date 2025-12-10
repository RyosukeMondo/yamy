#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QString>

/**
 * @brief Dialog for editing configuration metadata
 *
 * Allows users to view and edit:
 * - Name: Display name for the configuration
 * - Description: User description of the config
 * - Author: Who created the configuration
 * - Tags: Comma-separated list of tags
 * - Creation date (read-only)
 * - Modification date (read-only)
 */
class ConfigMetadataDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct metadata editor dialog
     * @param configPath Path to the configuration file
     * @param parent Parent widget
     */
    explicit ConfigMetadataDialog(const QString& configPath, QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ConfigMetadataDialog() override;

    /**
     * @brief Check if metadata was saved
     * @return true if Save was clicked and succeeded
     */
    bool wasSaved() const { return m_saved; }

private slots:
    /**
     * @brief Save the metadata and close
     */
    void onSave();

    /**
     * @brief Cancel and close without saving
     */
    void onCancel();

    /**
     * @brief Validate input fields
     */
    void validateInput();

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();

    /**
     * @brief Load metadata from file
     */
    void loadMetadata();

    /**
     * @brief Format a timestamp for display
     * @param timestamp Unix timestamp
     * @return Formatted date string
     */
    QString formatTimestamp(time_t timestamp) const;

    // Path to the config file
    QString m_configPath;

    // Whether metadata was saved
    bool m_saved;

    // UI Components
    QLineEdit* m_editName;
    QTextEdit* m_editDescription;
    QLineEdit* m_editAuthor;
    QLineEdit* m_editTags;
    QLabel* m_labelCreated;
    QLabel* m_labelModified;
    QPushButton* m_btnSave;
    QPushButton* m_btnCancel;
    QLabel* m_labelValidation;
};
