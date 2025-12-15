#include "lock_indicator_widget.h"

#include <QHBoxLayout>
#include <QString>
#include <cstring>  // for std::memcpy

LockIndicatorWidget::LockIndicatorWidget(QWidget* parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
    , m_lastLockBits{0}
    , m_statusLabel(new QLabel("Locks: 0 active", this))
{
    m_layout->setContentsMargins(5, 5, 5, 5);
    m_layout->setSpacing(3);

    // Add status label at top
    m_statusLabel->setStyleSheet("QLabel { font-weight: bold; color: #666; }");
    m_layout->addWidget(m_statusLabel);

    m_layout->addStretch();  // Push indicators to top
    setLayout(m_layout);
}

void LockIndicatorWidget::updateLockStatus(const uint32_t lockBits[8])
{
    // Optimize: Only update locks that changed state
    for (uint8_t wordIdx = 0; wordIdx < 8; ++wordIdx) {
        uint32_t changed = lockBits[wordIdx] ^ m_lastLockBits[wordIdx];
        if (changed == 0) {
            continue;  // No changes in this word
        }

        // Check each bit that changed
        for (uint8_t bitIdx = 0; bitIdx < 32; ++bitIdx) {
            if (changed & (1u << bitIdx)) {
                uint8_t lockNum = wordIdx * 32 + bitIdx;
                bool isActive = isLockBitSet(lockBits, lockNum);
                setLockActive(lockNum, isActive);
            }
        }
    }

    // Update existing indicators that weren't in the changed set
    // (in case indicator exists but lock stayed same)
    for (auto it = m_indicators.begin(); it != m_indicators.end(); ++it) {
        uint8_t lockNum = it.key();
        bool isActive = isLockBitSet(lockBits, lockNum);
        bool wasActive = isLockBitSet(m_lastLockBits, lockNum);

        // Only update if state actually changed
        if (isActive != wasActive) {
            setLockActive(lockNum, isActive);
        }
    }

    // Store current state for next delta
    std::memcpy(m_lastLockBits, lockBits, sizeof(m_lastLockBits));

    // Update status label with active count
    int activeCount = 0;
    for (uint8_t wordIdx = 0; wordIdx < 8; ++wordIdx) {
        uint32_t word = lockBits[wordIdx];
        while (word) {
            activeCount += word & 1;
            word >>= 1;
        }
    }
    m_statusLabel->setText(QString("Locks: %1 active").arg(activeCount));
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

    // Update tooltip
    QString tooltip = QString("Lock L%1: %2")
        .arg(lockNum, 2, 16, QChar('0'))
        .arg(active ? "ACTIVE" : "Inactive")
        .toUpper();
    indicator.container->setToolTip(tooltip);

    // Update indicator color based on active state
    if (active) {
        // Green for active with smooth transition
        indicator.indicator->setStyleSheet(
            "QLabel { "
            "  background-color: #00C853; "
            "  border-radius: 8px; "
            "  color: #00C853; "
            "}"
        );
        indicator.container->setVisible(true);
    } else {
        // Gray for inactive
        indicator.indicator->setStyleSheet(
            "QLabel { "
            "  background-color: #9E9E9E; "
            "  border-radius: 8px; "
            "  color: #9E9E9E; "
            "}"
        );
        // Hide inactive indicators to reduce clutter
        indicator.container->setVisible(false);
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
