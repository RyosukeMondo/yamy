#pragma once

#include <QDialog>

// Forward declarations for pointers
class QListWidget;
class QListWidgetItem;
class QPushButton;

/**
 * @brief Dialog for managing YAMY configuration files.
 *
 * Allows users to:
 * - View a list of available configurations.
 * - Create, delete, and rename configurations.
 * - Set a configuration as active.
 * - Open a configuration for editing.
 */
class ConfigManagerDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct the configuration manager dialog.
     * @param parent Parent widget.
     */
    explicit ConfigManagerDialog(QWidget* parent = nullptr);
    ~ConfigManagerDialog() override = default;

private slots:
    /** @brief Handles the 'New' button click. */
    void onNew();

    /** @brief Handles the 'Delete' button click. */
    void onDelete();

    /** @brief Handles the 'Rename' button click. */
    void onRename();

    /** @brief Handles the 'Set Active' button click. */
    void onSetActive();

    /** @brief Handles the 'Edit' button click. */
    void onEdit();

    /** @brief Updates UI elements when the list selection changes. */
    void onSelectionChanged();

    /**
     * @brief Handles double-clicking an item in the list.
     * @param item The item that was double-clicked.
     */
    void onItemDoubleClicked(QListWidgetItem* item);

private:
    /** @brief Sets up the UI layout and widgets. */
    void setupUI();

    /** @brief Populates the list with mock data. */
    void populateList();

    /** @brief Enables or disables buttons based on the current selection. */
    void updateButtonStates();

    // UI Widgets
    QListWidget* m_configList;
    QPushButton* m_btnNew;
    QPushButton* m_btnDelete;
    QPushButton* m_btnRename;
    QPushButton* m_btnSetActive;
    QPushButton* m_btnEdit;
};
