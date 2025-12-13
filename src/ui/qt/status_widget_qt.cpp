#include "status_widget_qt.h"
#include "../../core/engine/engine.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

StatusWidgetQt::StatusWidgetQt(Engine* engine, QWidget* parent)
    : QWidget(parent)
    , m_engine(engine)
    , m_updateTimer(new QTimer(this))
    , m_startTime(QDateTime::currentDateTime())
{
    setupUI();

    // Update every second
    connect(m_updateTimer, &QTimer::timeout, this, &StatusWidgetQt::updateStats);
    m_updateTimer->start(1000);
    
    updateStats();
}

StatusWidgetQt::~StatusWidgetQt()
{
}

void StatusWidgetQt::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* group = new QGroupBox("System Status");
    QVBoxLayout* groupLayout = new QVBoxLayout(group);

    // Uptime
    QHBoxLayout* uptimeLayout = new QHBoxLayout();
    uptimeLayout->addWidget(new QLabel("Uptime:"));
    m_labelUptime = new QLabel("-");
    uptimeLayout->addWidget(m_labelUptime);
    uptimeLayout->addStretch();
    groupLayout->addLayout(uptimeLayout);

    // Active Window
    QHBoxLayout* windowLayout = new QHBoxLayout();
    windowLayout->addWidget(new QLabel("Active Window:"));
    m_labelActiveWindow = new QLabel("-");
    windowLayout->addWidget(m_labelActiveWindow);
    windowLayout->addStretch();
    groupLayout->addLayout(windowLayout);

    // Keys (Placeholder for now as Engine might not expose counter directly yet)
    // We can add it back when metrics are available
    /*
    QHBoxLayout* keysLayout = new QHBoxLayout();
    keysLayout->addWidget(new QLabel("Keys:"));
    m_labelKeysProcessed = new QLabel("0");
    keysLayout->addWidget(m_labelKeysProcessed);
    keysLayout->addStretch();
    groupLayout->addLayout(keysLayout);
    */

    mainLayout->addWidget(group);
}

void StatusWidgetQt::updateStats()
{
    // Update Uptime
    qint64 secs = m_startTime.secsTo(QDateTime::currentDateTime());
    int days = secs / 86400;
    int hours = (secs % 86400) / 3600;
    int mins = (secs % 3600) / 60;
    int s = secs % 60;
    
    QString uptimeStr;
    if (days > 0) uptimeStr += QString("%1d ").arg(days);
    if (hours > 0 || days > 0) uptimeStr += QString("%1h ").arg(hours);
    uptimeStr += QString("%1m %2s").arg(mins).arg(s);
    
    m_labelUptime->setText(uptimeStr);

    // Update Active Window if Engine available
    // Note: Engine threading might require care, but for reading stats it's usually fine
    // or we should use IPC. For this simple widget, we'll leave it as placeholder
    // or use a safe method if available.
    if (m_engine) {
        // TODO: Get real active window name safely
        m_labelActiveWindow->setText("(running)");
    }
}
