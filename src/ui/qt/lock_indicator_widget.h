#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QMap>
#include <cstdint>

/**
 * @brief Widget to display lock status indicators for L00-LFF lock keys.
 *
 * Creates indicators on demand (not all 256 upfront) and displays only
 * active or recently used locks. Updates colors based on lock state:
 * - Green = active
 * - Gray = inactive
 */
class LockIndicatorWidget : public QWidget {
    Q_OBJECT

public:
    explicit LockIndicatorWidget(QWidget* parent = nullptr);
    virtual ~LockIndicatorWidget() = default;

    /**
     * @brief Update lock status from IPC message.
     * @param lockBits Array of 8 uint32_t values representing 256 lock states (bit set = active)
     */
    void updateLockStatus(const uint32_t lockBits[8]);

private:
    struct LockIndicator {
        QLabel* label;      // "Lxx" text label
        QLabel* indicator;  // Colored dot indicator
        QWidget* container; // Container for horizontal layout
    };

    /**
     * @brief Create or get indicator for a specific lock number.
     * @param lockNum Lock number (0-255 for L00-LFF)
     * @return Reference to LockIndicator struct
     */
    LockIndicator& getOrCreateIndicator(uint8_t lockNum);

    /**
     * @brief Set the active state of a lock indicator.
     * @param lockNum Lock number (0-255)
     * @param active True if lock is active (green), false if inactive (gray)
     */
    void setLockActive(uint8_t lockNum, bool active);

    /**
     * @brief Check if a specific bit is set in the lock bits array.
     * @param lockBits Array of 8 uint32_t values
     * @param lockNum Lock number (0-255)
     * @return True if the lock bit is set, false otherwise
     */
    static bool isLockBitSet(const uint32_t lockBits[8], uint8_t lockNum);

    QVBoxLayout* m_layout;
    QMap<uint8_t, LockIndicator> m_indicators;  // On-demand created indicators
};
