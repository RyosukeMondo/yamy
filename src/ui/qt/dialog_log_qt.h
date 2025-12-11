#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFontComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
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
    void onPauseResume();
    void onExport();
    void onClose();
    void onLevelFilterChanged(int index);
    void onCategoryFilterChanged(bool checked);
    void onFontFamilyChanged(const QFont& font);
    void onFontSizeChanged(int size);
    void onSearchTextChanged(const QString& text);
    void onFindNext();
    void onFindPrevious();
    void onCaseSensitiveToggled(bool checked);
    void onBufferLimitChanged(int value);

private:
    struct CachedLogEntry {
        yamy::logging::LogLevel level;
        QString category;
        QString plainText;
        QString htmlText;
    };

    void setupUI();
    void setupFilterControls(QVBoxLayout* mainLayout);
    void setupFontControls(QHBoxLayout* filterLayout);
    void subscribeToLogger();
    void scrollToBottom();
    void rebuildLogView();
    void processLogEntry(const CachedLogEntry& entry);
    bool shouldDisplay(const CachedLogEntry& entry) const;
    QString formatLogEntry(const yamy::logging::LogEntry& entry) const;
    QString formatLogEntryHtml(const yamy::logging::LogEntry& entry) const;
    static QString escapeHtml(const QString& text);
    static QString highlightKeywords(const QString& text);
    void loadFontSettings();
    void saveFontSettings();
    void applyFont();
    void loadBufferSettings();
    void saveBufferSettings();
    void trimBufferIfNeeded();
    void updateBufferUsageDisplay();
    void updatePauseIndicator();
    void setupSearchControls(QVBoxLayout* mainLayout);
    void highlightAllMatches();
    void clearSearchHighlights();
    void updateSearchStatus();
    int countMatches();
    void findMatch(bool forward);

    // Filter controls
    QComboBox* m_levelFilter;
    QGroupBox* m_categoryGroup;
    QHash<QString, QCheckBox*> m_categoryFilters;

    // Font controls
    QFontComboBox* m_fontCombo;
    QSpinBox* m_fontSizeSpinner;

    // Buffer limit control
    QSpinBox* m_bufferLimitSpinner;

    // UI Components
    yamy::ui::LogStatsPanel* m_statsPanel;
    QTextEdit* m_logView;
    QPushButton* m_btnClear;
    QPushButton* m_btnPause;
    QPushButton* m_btnSave;
    QPushButton* m_btnClose;
    QLabel* m_pauseIndicator;

    // Search controls
    QLineEdit* m_searchEdit;
    QPushButton* m_btnFindNext;
    QPushButton* m_btnFindPrev;
    QCheckBox* m_caseSensitive;
    QLabel* m_searchStatus;

    // State
    bool m_paused;
    int m_entriesWhilePaused;
    yamy::logging::LogLevel m_minLevel;
    std::vector<CachedLogEntry> m_allEntries;

    // Search state
    QString m_searchText;
    bool m_searchCaseSensitive;
    int m_currentMatchIndex;
    int m_totalMatches;

    // Buffer limit
    int m_maxBufferSize;
    static constexpr int DEFAULT_MAX_BUFFER_SIZE = 10000;
    static constexpr int MIN_BUFFER_SIZE = 1000;
    static constexpr int MAX_BUFFER_SIZE = 100000;

    // Standard categories
    static constexpr const char* CATEGORIES[] = {
        "Engine", "Parser", "Input", "Window", "Config"
    };
    static constexpr size_t CATEGORY_COUNT = 5;
};
