#pragma once

#include <QDialog>
#include <QFileSystemWatcher>
#include <QHash>
#include <QIcon>
#include <QMutex>
#include <QSet>
#include <QThread>
#include <QTimer>

// Forward declarations for pointers
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QTextEdit;

/**
 * @brief Worker class for background config validation
 */
class ConfigValidationWorker : public QObject {
    Q_OBJECT

public:
    explicit ConfigValidationWorker(QObject* parent = nullptr);

public slots:
    void validateConfig(const QString& configPath);

signals:
    void validationComplete(const QString& configPath,
                            bool hasErrors,
                            bool hasWarnings,
                            const QStringList& errorMessages);
};

/**
 * @brief Dialog for managing YAMY configuration files.
 *
 * Allows users to:
 * - View a list of available configurations with validation status
 * - Create, delete, duplicate, and rename configurations
 * - Set a configuration as active
 * - Open a configuration for editing
 * - Edit configuration metadata
 * - Import/export configurations
 * - Search and filter configurations
 */
class ConfigManagerDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct the configuration manager dialog.
     * @param parent Parent widget.
     */
    explicit ConfigManagerDialog(QWidget* parent = nullptr);
    ~ConfigManagerDialog() override;

private slots:
    /** @brief Handles the 'New' button click. */
    void onNew();

    /** @brief Handles the 'Duplicate' button click. */
    void onDuplicate();

    /** @brief Handles the 'Delete' button click. */
    void onDelete();

    /** @brief Handles the 'Rename' button click. */
    void onRename();

    /** @brief Handles the 'Set Active' button click. */
    void onSetActive();

    /** @brief Handles the 'Edit' button click. */
    void onEdit();

    /** @brief Handles the 'Metadata' button click. */
    void onEditMetadata();

    /** @brief Handles the 'Import' button click. */
    void onImport();

    /** @brief Handles the 'Export' button click. */
    void onExport();

    /** @brief Updates UI elements when the list selection changes. */
    void onSelectionChanged();

    /**
     * @brief Handles double-clicking an item in the list.
     * @param item The item that was double-clicked.
     */
    void onItemDoubleClicked(QListWidgetItem* item);

    /** @brief Handles context menu request. */
    void onContextMenuRequested(const QPoint& pos);

    /** @brief Handles file change notification. */
    void onFileChanged(const QString& path);

    /** @brief Handles validation completion. */
    void onValidationComplete(const QString& configPath,
                              bool hasErrors,
                              bool hasWarnings,
                              const QStringList& errorMessages);

    /** @brief Handles search text change. */
    void onSearchTextChanged(const QString& text);

    /** @brief Handles status filter change. */
    void onStatusFilterChanged(int index);

    /** @brief Clears the search field. */
    void onClearSearch();

private:
    /** @brief Sets up the UI layout and widgets. */
    void setupUI();

    /** @brief Refreshes the config list from ConfigManager. */
    void refreshConfigList();

    /** @brief Enables or disables buttons based on the current selection. */
    void updateButtonStates();

    /** @brief Gets the path of the currently selected config. */
    QString getSelectedConfigPath() const;

    /** @brief Gets the index of the currently selected config. */
    int getSelectedIndex() const;

    /** @brief Validates a config name for invalid characters. */
    bool validateConfigName(const QString& name) const;

    /** @brief Opens a file in the configured editor. */
    void openInEditor(const QString& path);

    /** @brief Launches an editor with the given command. */
    bool launchEditor(const QString& editorCmd, const QString& filePath);

    /** @brief Creates validation status icons. */
    void createValidationIcons();

    /** @brief Gets the icon for validation status. */
    QIcon getValidationIcon(bool hasErrors, bool hasWarnings) const;

    /** @brief Sets up the file system watcher. */
    void setupFileWatcher();

    /** @brief Starts validation for a single config. */
    void startValidation(const QString& configPath);

    /** @brief Starts validation for all configs. */
    void startValidationForAll();

    /** @brief Updates list item validation status icon. */
    void updateItemValidationStatus(const QString& configPath,
                                    bool hasErrors,
                                    bool hasWarnings);

    /** @brief Checks if a config matches the current filter. */
    bool matchesFilter(const QString& configPath, const QString& displayName) const;

    /** @brief Applies search and status filters to the list. */
    void applyFilters();

    // Search and filter widgets
    QLineEdit* m_searchEdit;
    QPushButton* m_btnClearSearch;
    QComboBox* m_statusFilter;

    // Config list widget
    QListWidget* m_configList;

    // Action buttons
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

    // Status and info labels
    QLabel* m_labelStatus;
    QLabel* m_labelPath;
    QTextEdit* m_validationDetails;

    // File watcher for auto-revalidation
    QFileSystemWatcher* m_fileWatcher;
    QTimer* m_validationDebounceTimer;
    QSet<QString> m_pendingValidations;

    // Background validation
    QThread* m_validationThread;
    ConfigValidationWorker* m_validationWorker;

    // Validation cache
    struct ValidationStatus {
        bool hasErrors = false;
        bool hasWarnings = false;
        QStringList messages;
    };
    QHash<QString, ValidationStatus> m_validationCache;
    mutable QMutex m_validationCacheMutex;

    // Validation icons
    QIcon m_iconValid;
    QIcon m_iconWarning;
    QIcon m_iconError;
    QIcon m_iconPending;
};
