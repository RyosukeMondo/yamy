#include "lock_indicator_widget.h"

#include <QHBoxLayout>
#include <QString>

LockIndicatorWidget::LockIndicatorWidget(QWidget* parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(5, 5, 5, 5);
    m_layout->setSpacing(3);
    m_layout->addStretch();  // Push indicators to top
    setLayout(m_layout);
}

void LockIndicatorWidget::updateLockStatus(const uint32_t lockBits[8])
{
    // Track which locks are currently active
    for (uint16_t lockNum = 0; lockNum < 256; ++lockNum) {
        bool isActive = isLockBitSet(lockBits, static_cast<uint8_t>(lockNum));

        // Only create/update indicators for active locks or already existing indicators
        if (isActive || m_indicators.contains(static_cast<uint8_t>(lockNum))) {
            setLockActive(static_cast<uint8_t>(lockNum), isActive);
        }
    }
}

LockIndicatorWidget::LockIndicator& LockIndicatorWidget::getOrCreateIndicator(uint8_t lockNum)
{
    if (!m_indicators.contains(lockNum)) {
        // Create new indicator
        LockIndicator indicator;

        // Create container widget for horizontal layout
        indicator.container = new QWidget(this);
        QHBoxLayout* hLayout = new QHBoxLayout(indicator.container);
        hLayout->setContentsMargins(2, 2, 2, 2);
        hLayout->setSpacing(5);

        // Create label (e.g., "L00", "L0A", "LFF")
        indicator.label = new QLabel(QString("L%1").arg(lockNum, 2, 16, QChar('0')).toUpper(), indicator.container);
        indicator.label->setMinimumWidth(30);

        // Create colored dot indicator
        indicator.indicator = new QLabel("●", indicator.container);
        indicator.indicator->setMinimumSize(16, 16);
        indicator.indicator->setMaximumSize(16, 16);
        indicator.indicator->setAlignment(Qt::AlignCenter);

        // Add to layout
        hLayout->addWidget(indicator.label);
        hLayout->addWidget(indicator.indicator);
        hLayout->addStretch();

        // Insert before the stretch at the end
        int insertPos = m_layout->count() - 1;
        m_layout->insertWidget(insertPos, indicator.container);

        // Store in map
        m_indicators.insert(lockNum, indicator);
    }

    return m_indicators[lockNum];
}

void LockIndicatorWidget::setLockActive(uint8_t lockNum, bool active)
{
    LockIndicator& indicator = getOrCreateIndicator(lockNum);

    // Update indicator color based on active state
    if (active) {
        // Green for active
        indicator.indicator->setStyleSheet(
            "QLabel { background-color: #00C853; border-radius: 8px; color: #00C853; }"
        );
    } else {
        // Gray for inactive
        indicator.indicator->setStyleSheet(
            "QLabel { background-color: #9E9E9E; border-radius: 8px; color: #9E9E9E; }"
        );
    }
}

bool LockIndicatorWidget::isLockBitSet(const uint32_t lockBits[8], uint8_t lockNum)
{
    // Calculate which word and which bit
    uint8_t wordIdx = lockNum / 32;
    uint8_t bitIdx = lockNum % 32;

    // Check if the bit is set
    return (lockBits[wordIdx] & (1u << bitIdx)) != 0;
}
