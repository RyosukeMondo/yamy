#include "log_stats_panel.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace yamy {
namespace ui {

LogStatsPanel::LogStatsPanel(QWidget* parent)
    : QWidget(parent)
    , m_traceCount(0)
    , m_infoCount(0)
    , m_warningCount(0)
    , m_errorCount(0)
    , m_bufferCurrent(0)
    , m_bufferMax(10000)
    , m_groupBox(nullptr)
    , m_levelStatsLabel(nullptr)
    , m_categoryStatsLabel(nullptr)
    , m_bufferLabel(nullptr)
    , m_clearButton(nullptr)
    , m_collapsed(false)
{
    // Initialize category counters
    for (size_t i = 0; i < CATEGORY_COUNT; ++i) {
        m_categoryCounters[CATEGORIES[i]] = new std::atomic<int>(0);
    }

    setupUI();
    updateUI();
}

LogStatsPanel::~LogStatsPanel()
{
    // Clean up category counters
    for (auto* counter : m_categoryCounters) {
        delete counter;
    }
    m_categoryCounters.clear();
}

void LogStatsPanel::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Collapsible group box
    m_groupBox = new QGroupBox("Statistics");
    m_groupBox->setCheckable(true);
    m_groupBox->setChecked(true);
    connect(m_groupBox, &QGroupBox::toggled, this, &LogStatsPanel::setCollapsed);

    auto* groupLayout = new QHBoxLayout(m_groupBox);
    groupLayout->setContentsMargins(5, 2, 5, 2);

    // Level statistics label
    m_levelStatsLabel = new QLabel();
    m_levelStatsLabel->setTextFormat(Qt::RichText);
    groupLayout->addWidget(m_levelStatsLabel);

    groupLayout->addSpacing(20);

    // Category statistics label
    m_categoryStatsLabel = new QLabel();
    m_categoryStatsLabel->setTextFormat(Qt::RichText);
    groupLayout->addWidget(m_categoryStatsLabel);

    groupLayout->addSpacing(20);

    // Buffer usage label
    m_bufferLabel = new QLabel();
    groupLayout->addWidget(m_bufferLabel);

    groupLayout->addStretch();

    // Clear Stats button
    m_clearButton = new QPushButton("Clear Stats");
    m_clearButton->setMaximumWidth(100);
    connect(m_clearButton, &QPushButton::clicked, this, &LogStatsPanel::onClearStats);
    groupLayout->addWidget(m_clearButton);

    mainLayout->addWidget(m_groupBox);
}

void LogStatsPanel::incrementTrace()
{
    m_traceCount.fetch_add(1, std::memory_order_relaxed);
    QMetaObject::invokeMethod(this, "updateUI", Qt::QueuedConnection);
}

void LogStatsPanel::incrementInfo()
{
    m_infoCount.fetch_add(1, std::memory_order_relaxed);
    QMetaObject::invokeMethod(this, "updateUI", Qt::QueuedConnection);
}

void LogStatsPanel::incrementWarning()
{
    m_warningCount.fetch_add(1, std::memory_order_relaxed);
    QMetaObject::invokeMethod(this, "updateUI", Qt::QueuedConnection);
}

void LogStatsPanel::incrementError()
{
    m_errorCount.fetch_add(1, std::memory_order_relaxed);
    QMetaObject::invokeMethod(this, "updateUI", Qt::QueuedConnection);
}

void LogStatsPanel::incrementCategory(const QString& category)
{
    auto it = m_categoryCounters.find(category);
    if (it != m_categoryCounters.end()) {
        (*it)->fetch_add(1, std::memory_order_relaxed);
        QMetaObject::invokeMethod(this, "updateUI", Qt::QueuedConnection);
    }
}

void LogStatsPanel::setBufferUsage(int current, int max)
{
    m_bufferCurrent.store(current, std::memory_order_relaxed);
    m_bufferMax.store(max, std::memory_order_relaxed);
    QMetaObject::invokeMethod(this, "updateUI", Qt::QueuedConnection);
}

int LogStatsPanel::totalCount() const
{
    return m_traceCount.load(std::memory_order_relaxed) +
           m_infoCount.load(std::memory_order_relaxed) +
           m_warningCount.load(std::memory_order_relaxed) +
           m_errorCount.load(std::memory_order_relaxed);
}

void LogStatsPanel::reset()
{
    m_traceCount.store(0, std::memory_order_relaxed);
    m_infoCount.store(0, std::memory_order_relaxed);
    m_warningCount.store(0, std::memory_order_relaxed);
    m_errorCount.store(0, std::memory_order_relaxed);

    for (auto* counter : m_categoryCounters) {
        counter->store(0, std::memory_order_relaxed);
    }

    updateUI();
}

void LogStatsPanel::setCollapsed(bool collapsed)
{
    m_collapsed = !collapsed;  // QGroupBox checked=true means expanded
    if (m_collapsed) {
        m_levelStatsLabel->hide();
        m_categoryStatsLabel->hide();
        m_bufferLabel->hide();
        m_clearButton->hide();
    } else {
        m_levelStatsLabel->show();
        m_categoryStatsLabel->show();
        m_bufferLabel->show();
        m_clearButton->show();
    }
}

void LogStatsPanel::toggleCollapsed()
{
    m_groupBox->setChecked(!m_groupBox->isChecked());
}

void LogStatsPanel::onClearStats()
{
    reset();
    emit clearStatsRequested();
}

void LogStatsPanel::updateUI()
{
    updateLevelDisplay();
    updateCategoryDisplay();
    updateBufferDisplay();
}

void LogStatsPanel::updateLevelDisplay()
{
    int trace = m_traceCount.load(std::memory_order_relaxed);
    int info = m_infoCount.load(std::memory_order_relaxed);
    int warning = m_warningCount.load(std::memory_order_relaxed);
    int error = m_errorCount.load(std::memory_order_relaxed);
    int total = trace + info + warning + error;

    QStringList parts;

    // Total always shown
    parts << QString("<b>Total:</b> %1").arg(total);

    // Only show non-zero counts
    if (error > 0) {
        parts << QString("<span style='color:#FF0000;'><b>Errors:</b> %1</span>").arg(error);
    }
    if (warning > 0) {
        parts << QString("<span style='color:#FFA500;'><b>Warnings:</b> %1</span>").arg(warning);
    }
    if (info > 0) {
        parts << QString("<b>Info:</b> %1").arg(info);
    }
    if (trace > 0) {
        parts << QString("<span style='color:#808080;'><b>Trace:</b> %1</span>").arg(trace);
    }

    m_levelStatsLabel->setText(parts.join(" | "));
}

void LogStatsPanel::updateCategoryDisplay()
{
    QStringList parts;

    for (size_t i = 0; i < CATEGORY_COUNT; ++i) {
        const char* cat = CATEGORIES[i];
        auto it = m_categoryCounters.find(cat);
        if (it != m_categoryCounters.end()) {
            int count = (*it)->load(std::memory_order_relaxed);
            if (count > 0) {
                parts << QString("%1: %2").arg(cat).arg(count);
            }
        }
    }

    if (parts.isEmpty()) {
        m_categoryStatsLabel->setText("<i>No entries by category</i>");
    } else {
        m_categoryStatsLabel->setText(parts.join(" | "));
    }
}

void LogStatsPanel::updateBufferDisplay()
{
    int current = m_bufferCurrent.load(std::memory_order_relaxed);
    int max = m_bufferMax.load(std::memory_order_relaxed);

    // Color based on buffer usage
    QString color;
    double usage = static_cast<double>(current) / static_cast<double>(max);
    if (usage > 0.9) {
        color = "#FF0000";  // Red when > 90% full
    } else if (usage > 0.75) {
        color = "#FFA500";  // Orange when > 75% full
    }

    QString text = QString("Buffer: %1/%2").arg(current).arg(max);
    if (!color.isEmpty()) {
        m_bufferLabel->setText(QString("<span style='color:%1;'>%2</span>").arg(color, text));
    } else {
        m_bufferLabel->setText(text);
    }
}

}  // namespace ui
}  // namespace yamy
