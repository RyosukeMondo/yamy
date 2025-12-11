#include "log_stats_panel.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

namespace yamy
{
namespace ui
{
LogStatsPanel::LogStatsPanel(QWidget* parent) :
    QWidget(parent),
    m_errorCount(0),
    m_warningCount(0),
    m_totalLines(0),
    m_errorCountLabel(new QLabel(this)),
    m_warningCountLabel(new QLabel(this)),
    m_totalLinesLabel(new QLabel(this))
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(new QLabel("Errors:", this));
    layout->addWidget(m_errorCountLabel);
    layout->addWidget(new QLabel("Warnings:", this));
    layout->addWidget(m_warningCountLabel);
    layout->addWidget(new QLabel("Total Lines:", this));
    layout->addWidget(m_totalLinesLabel);
    layout->addStretch();

    reset();
}

LogStatsPanel::~LogStatsPanel()
{
}

void LogStatsPanel::incrementError()
{
    m_errorCount++;
    updateUi();
}

void LogStatsPanel::incrementWarning()
{
    m_warningCount++;
    updateUi();
}

void LogStatsPanel::setTotalLines(int count)
{
    m_totalLines = count;
    updateUi();
}

void LogStatsPanel::reset()
{
    m_errorCount = 0;
    m_warningCount = 0;
    m_totalLines = 0;
    updateUi();
}

void LogStatsPanel::updateUi()
{
    m_errorCountLabel->setText(QString::number(m_errorCount));
    m_warningCountLabel->setText(QString::number(m_warningCount));
    m_totalLinesLabel->setText(QString::number(m_totalLines));
}
}
}
