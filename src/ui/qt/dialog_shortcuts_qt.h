#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

/**
 * @brief Keyboard shortcuts reference dialog
 *
 * Shows a searchable table of all keyboard shortcuts:
 * - Global hotkeys (from configuration)
 * - Dialog shortcuts
 * - Application shortcuts
 */
class DialogShortcutsQt : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct shortcuts dialog
     * @param parent Parent widget
     */
    explicit DialogShortcutsQt(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~DialogShortcutsQt() override;

private slots:
    /**
     * @brief Filter shortcuts based on search text
     */
    void onSearchTextChanged(const QString& text);

    /**
     * @brief Close dialog
     */
    void onClose();

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();

    /**
     * @brief Load shortcuts into the table
     */
    void loadShortcuts();

    /**
     * @brief Add a shortcut entry to the table
     * @param action Action description
     * @param shortcut Shortcut key sequence
     * @param category Category (Global, Dialog, Application)
     */
    void addShortcut(const QString& action, const QString& shortcut,
                     const QString& category);

    // UI Components
    QLineEdit* m_searchBox;
    QTableWidget* m_shortcutTable;
    QPushButton* m_btnClose;
};
