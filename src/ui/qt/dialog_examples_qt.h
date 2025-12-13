#pragma once

#include <QDialog>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>

/**
 * @brief Configuration examples dialog
 *
 * Shows example .mayu configuration files:
 * - Example list on the left
 * - Example code on the right (read-only)
 * - Copy to clipboard and Save As functionality
 */
class DialogExamplesQt : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct examples dialog
     * @param parent Parent widget
     */
    explicit DialogExamplesQt(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~DialogExamplesQt() override;

private slots:
    /**
     * @brief Handle example selection change
     */
    void onExampleSelected(int index);

    /**
     * @brief Copy current example to clipboard
     */
    void onCopyToClipboard();

    /**
     * @brief Save current example to file
     */
    void onSaveAs();

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
     * @brief Load example list
     */
    void loadExamples();

    /**
     * @brief Get example code by index
     * @param index Example index
     * @return Example .mayu configuration code
     */
    QString getExampleCode(int index) const;

    // UI Components
    QListWidget* m_exampleList;
    QTextEdit* m_codeView;
    QPushButton* m_btnCopy;
    QPushButton* m_btnSaveAs;
    QPushButton* m_btnClose;
};
