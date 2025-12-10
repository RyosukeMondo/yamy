#pragma once

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QIcon>
#include <QMap>
#include <QSet>
#include <QMutex>
#include <QMenu>
#include <memory>

class ConfigManager;
class QThread;
class ConfigValidationWorker;

/**
 * @brief Dialog for managing YAMY configuration files
 *
 * Allows users to:
 * - View list of available configurations
 * - Add new configurations (from file or template)
 * - Duplicate existing configurations
 * - Delete configurations
 * - Rename configurations
 * - Edit configurations in external editor
 * - Import/Export configurations
 * - Set active configuration
 */
class ConfigManagerDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct configuration manager dialog
     * @param parent Parent widget
     */
    explicit ConfigManagerDialog(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ConfigManagerDialog() override;

private slots:
    /**
     * @brief Add a new configuration file
     */
    void onNew();

    /**
     * @brief Duplicate the selected configuration
     */
    void onDuplicate();

    /**
     * @brief Delete the selected configuration
     */
    void onDelete();

    /**
     * @brief Rename the selected configuration
     */
    void onRename();

    /**
     * @brief Edit the selected configuration in external editor
     */
    void onEdit();

    /**
     * @brief Import a configuration from archive
     */
    void onImport();

    /**
     * @brief Export the selected configuration to archive
     */
    void onExport();

    /**
     * @brief Set the selected configuration as active
     */
    void onSetActive();

    /**
     * @brief Handle selection change in the config list
     */
    void onSelectionChanged();

    /**
     * @brief Handle double-click on a config item
     * @param item The double-clicked item
     */
    void onItemDoubleClicked(QListWidgetItem* item);

    /**
     * @brief Show context menu for config item
     * @param pos Position where context menu was requested
     */
    void onContextMenuRequested(const QPoint& pos);

    /**
     * @brief Edit metadata for the selected configuration
     */
    void onEditMetadata();

    /**
     * @brief Refresh the configuration list
     */
    void refreshConfigList();

    /**
     * @brief Handle validation results from worker thread
     * @param configPath Path of validated config
     * @param hasErrors True if validation found errors
     * @param hasWarnings True if validation found warnings
     * @param errorMessages Formatted error/warning messages
     */
    void onValidationComplete(const QString& configPath, bool hasErrors,
                              bool hasWarnings, const QStringList& errorMessages);

    /**
     * @brief Handle file changes detected by watcher
     * @param path Path to changed file
     */
    void onFileChanged(const QString& path);

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();

    /**
     * @brief Update button states based on selection
     */
    void updateButtonStates();

    /**
     * @brief Get the path of the selected configuration
     * @return Path to selected config, or empty string if none selected
     */
    QString getSelectedConfigPath() const;

    /**
     * @brief Get the index of the selected configuration
     * @return Index of selected config, or -1 if none selected
     */
    int getSelectedIndex() const;

    /**
     * @brief Validate a configuration name
     * @param name Name to validate
     * @return true if name is valid
     */
    bool validateConfigName(const QString& name) const;

    /**
     * @brief Open a configuration file in external editor
     * @param path Path to the configuration file
     *
     * Uses the following strategy to find an editor:
     * 1. User-configured editor command from settings
     * 2. $EDITOR environment variable
     * 3. $VISUAL environment variable
     * 4. QDesktopServices system default
     * 5. Platform-specific fallback (xdg-open on Linux, notepad on Windows)
     */
    void openInEditor(const QString& path);

    /**
     * @brief Launch an editor with the specified command
     * @param editorCmd Editor command (may contain %f placeholder for file path)
     * @param filePath Path to the file to open
     * @return true if editor was launched successfully
     */
    bool launchEditor(const QString& editorCmd, const QString& filePath);

    /**
     * @brief Start validation for a config file
     * @param configPath Path to config file
     */
    void startValidation(const QString& configPath);

    /**
     * @brief Start validation for all configs
     */
    void startValidationForAll();

    /**
     * @brief Get icon for validation status
     * @param hasErrors True if has errors
     * @param hasWarnings True if has warnings
     * @return Icon for the status
     */
    QIcon getValidationIcon(bool hasErrors, bool hasWarnings) const;

    /**
     * @brief Create validation status icons
     */
    void createValidationIcons();

    /**
     * @brief Update list item with validation status
     * @param configPath Path to config
     * @param hasErrors True if has errors
     * @param hasWarnings True if has warnings
     */
    void updateItemValidationStatus(const QString& configPath, bool hasErrors,
                                    bool hasWarnings);

    /**
     * @brief Setup file watcher for config files
     */
    void setupFileWatcher();

    // UI Components
    QListWidget* m_configList;
    QPushButton* m_btnNew;
    QPushButton* m_btnDuplicate;
    QPushButton* m_btnDelete;
    QPushButton* m_btnRename;
    QPushButton* m_btnEdit;
    QPushButton* m_btnMetadata;
    QPushButton* m_btnImport;
    QPushButton* m_btnExport;
    QPushButton* m_btnSetActive;
    QPushButton* m_btnClose;
    QLabel* m_labelStatus;
    QLabel* m_labelPath;
    QTextEdit* m_validationDetails;

    // Validation support
    QFileSystemWatcher* m_fileWatcher;
    QTimer* m_validationDebounceTimer;
    QThread* m_validationThread;
    ConfigValidationWorker* m_validationWorker;

    // Validation status icons
    QIcon m_iconValid;
    QIcon m_iconError;
    QIcon m_iconWarning;
    QIcon m_iconPending;

    // Validation results cache: configPath -> (hasErrors, hasWarnings, messages)
    struct ValidationStatus {
        bool hasErrors = false;
        bool hasWarnings = false;
        QStringList messages;
    };
    QMap<QString, ValidationStatus> m_validationCache;
    QMutex m_validationCacheMutex;

    // Pending validations (debounced)
    QSet<QString> m_pendingValidations;
};

/**
 * @brief Worker object for background config validation
 */
class ConfigValidationWorker : public QObject {
    Q_OBJECT

public:
    explicit ConfigValidationWorker(QObject* parent = nullptr);

public slots:
    /**
     * @brief Validate a config file
     * @param configPath Path to config file
     */
    void validateConfig(const QString& configPath);

signals:
    /**
     * @brief Emitted when validation completes
     * @param configPath Path of validated config
     * @param hasErrors True if validation found errors
     * @param hasWarnings True if validation found warnings
     * @param errorMessages Formatted error/warning messages
     */
    void validationComplete(const QString& configPath, bool hasErrors,
                           bool hasWarnings, const QStringList& errorMessages);
};
