#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHash>
#include <QPushButton>
#include <QString>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <vector>

#include "core/logging/log_entry.h"

namespace yamy {
namespace ui {
class LogStatsPanel;
}
}

/**
 * @brief Log viewer dialog with filtering capabilities
 *
 * Displays YAMY log messages with:
 * - Real-time log updates from Logger
 * - Level filtering (Trace, Info, Warning, Error)
 * - Category filtering (Engine, Parser, Input, Window, Config)
 * - Auto-scroll to latest messages
 * - Clear log functionality
 * - Save to file
 * - Thread-safe updates via QMetaObject::invokeMethod
 */
class DialogLogQt : public QDialog {
    Q_OBJECT

public:
    explicit DialogLogQt(QWidget* parent = nullptr);
    ~DialogLogQt() override;

public slots:
    /**
     * @brief Append formatted log message (called from UI thread)
     * @param message Log message to append
     */
    void appendLog(const QString& message);

    /**
     * @brief Thread-safe method to receive log entries from Logger
     * @param entry Log entry to process
     */
    void onLogEntry(const yamy::logging::LogEntry& entry);

    void clearLog();
    void setAutoScroll(bool enabled);

private slots:
    void onClear();
    void onSave();
    void onClose();
    void onAutoScrollToggled(bool checked);
    void onLevelFilterChanged(int index);
    void onCategoryFilterChanged(bool checked);

private:
    struct CachedLogEntry {
        yamy::logging::LogLevel level;
        QString category;
        QString formattedText;
    };

    void setupUI();
    void setupFilterControls(QVBoxLayout* mainLayout);
    void subscribeToLogger();
    void scrollToBottom();
    void rebuildLogView();
    void processLogEntry(const CachedLogEntry& entry);
    bool shouldDisplay(const CachedLogEntry& entry) const;
    QString formatLogEntry(const yamy::logging::LogEntry& entry) const;

    // Filter controls
    QComboBox* m_levelFilter;
    QGroupBox* m_categoryGroup;
    QHash<QString, QCheckBox*> m_categoryFilters;

    // UI Components
    yamy::ui::LogStatsPanel* m_statsPanel;
    QTextEdit* m_logView;
    QPushButton* m_btnClear;
    QPushButton* m_btnSave;
    QPushButton* m_btnClose;
    QCheckBox* m_chkAutoScroll;

    // State
    bool m_autoScroll;
    yamy::logging::LogLevel m_minLevel;
    std::vector<CachedLogEntry> m_allEntries;

    static constexpr int MAX_LOG_ENTRIES = 10000;

    // Standard categories
    static constexpr const char* CATEGORIES[] = {
        "Engine", "Parser", "Input", "Window", "Config"
    };
    static constexpr size_t CATEGORY_COUNT = 5;
};
