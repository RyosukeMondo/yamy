#pragma once

#include <QtWidgets/QWidget>

class QLabel;

namespace yamy
{
namespace ui
{
class LogStatsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit LogStatsPanel(QWidget* parent = 0);
    ~LogStatsPanel();

public slots:
    void incrementError();
    void incrementWarning();
    void setTotalLines(int count);
    void reset();

private:
    void updateUi();

private:
    int m_errorCount;
    int m_warningCount;
    int m_totalLines;

    QLabel* m_errorCountLabel;
    QLabel* m_warningCountLabel;
    QLabel* m_totalLinesLabel;
};
}
}
