#pragma once

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QString>
#include <QTimer>

/**
 * @brief Log viewer dialog
 *
 * Displays YAMY log messages with:
 * - Auto-scroll to latest messages
 * - Clear log functionality
 * - Save to file
 * - Real-time log updates
 */
class DialogLogQt : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct log dialog
     * @param parent Parent widget
     */
    explicit DialogLogQt(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~DialogLogQt() override;

public slots:
    /**
     * @brief Append log message
     * @param message Log message to append
     */
    void appendLog(const QString& message);

    /**
     * @brief Clear all log messages
     */
    void clearLog();

    /**
     * @brief Set auto-scroll enabled/disabled
     * @param enabled true to auto-scroll
     */
    void setAutoScroll(bool enabled);

private slots:
    /**
     * @brief Clear button clicked
     */
    void onClear();

    /**
     * @brief Save log to file
     */
    void onSave();

    /**
     * @brief Close dialog
     */
    void onClose();

    /**
     * @brief Auto-scroll toggle changed
     */
    void onAutoScrollToggled(bool checked);

    /**
     * @brief Update log from engine (periodic)
     */
    void onUpdateLog();

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();

    /**
     * @brief Scroll to bottom of log
     */
    void scrollToBottom();

    // UI Components
    QTextEdit* m_logView;
    QPushButton* m_btnClear;
    QPushButton* m_btnSave;
    QPushButton* m_btnClose;
    QCheckBox* m_chkAutoScroll;

    // State
    bool m_autoScroll;
    QTimer* m_updateTimer;

    // Maximum log lines to keep
    static const int MAX_LOG_LINES = 10000;
};
