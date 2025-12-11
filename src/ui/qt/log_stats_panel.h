#pragma once

#include <QHash>
#include <QWidget>
#include <atomic>

class QHBoxLayout;
class QLabel;
class QPushButton;
class QGroupBox;

namespace yamy {
namespace ui {

/**
 * @brief Collapsible statistics panel showing log entry counts
 *
 * Displays:
 * - Total entry count
 * - Counts by level (Trace, Info, Warning, Error)
 * - Counts by category (Engine, Parser, Input, Window, Config)
 * - Buffer usage indicator
 * - Clear Stats button to reset counters
 *
 * All counter updates are thread-safe using atomic operations.
 */
class LogStatsPanel : public QWidget {
    Q_OBJECT

public:
    explicit LogStatsPanel(QWidget* parent = nullptr);
    ~LogStatsPanel() override;

    // Level-specific increment methods (thread-safe)
    void incrementTrace();
    void incrementInfo();
    void incrementWarning();
    void incrementError();

    // Category-specific increment method (thread-safe)
    void incrementCategory(const QString& category);

    // Buffer usage display
    void setBufferUsage(int current, int max);

    // Get total count (thread-safe)
    int totalCount() const;

signals:
    void clearStatsRequested();

public slots:
    void reset();
    void setCollapsed(bool collapsed);
    void toggleCollapsed();

private slots:
    void onClearStats();

private:
    void setupUI();
    void updateUI();
    void updateLevelDisplay();
    void updateCategoryDisplay();
    void updateBufferDisplay();

    // Counters (atomic for thread safety)
    std::atomic<int> m_traceCount;
    std::atomic<int> m_infoCount;
    std::atomic<int> m_warningCount;
    std::atomic<int> m_errorCount;
    std::atomic<int> m_bufferCurrent;
    std::atomic<int> m_bufferMax;

    // Category counters
    QHash<QString, std::atomic<int>*> m_categoryCounters;

    // UI Components
    QGroupBox* m_groupBox;
    QLabel* m_levelStatsLabel;
    QLabel* m_categoryStatsLabel;
    QLabel* m_bufferLabel;
    QPushButton* m_clearButton;

    // Collapsed state
    bool m_collapsed;

    // Standard categories
    static constexpr const char* CATEGORIES[] = {
        "Engine", "Parser", "Input", "Window", "Config"
    };
    static constexpr size_t CATEGORY_COUNT = 5;
};

}  // namespace ui
}  // namespace yamy
