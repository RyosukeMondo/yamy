//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// lock_state.cpp - Lock key state management implementation

#include "lock_state.h"
#include <cstring>   // for memset
#include <cstdio>    // for fprintf (logging)

namespace yamy::input {

LockState::LockState()
    : m_locks{0}
{
    // Initialize all locks to inactive state
}

void LockState::toggleLock(uint8_t lock_num)
{
    // Calculate word index and bit index
    // lock_num 0-31 -> word 0, bit 0-31
    // lock_num 32-63 -> word 1, bit 0-31
    // etc.
    uint8_t word_idx = lock_num / 32;  // 0-7
    uint8_t bit_idx = lock_num % 32;   // 0-31
    uint32_t mask = 1u << bit_idx;

    // Toggle the bit using XOR
    m_locks[word_idx] ^= mask;

    // Check new state for logging
    bool newState = (m_locks[word_idx] & mask) != 0;

    fprintf(stderr, "[LockState] Lock L%02X toggled to %s\n",
            lock_num, newState ? "ACTIVE" : "INACTIVE");
    fflush(stderr);

    // Notify GUI of state change
    notifyGUI();
}

bool LockState::isLockActive(uint8_t lock_num) const
{
    uint8_t word_idx = lock_num / 32;
    uint8_t bit_idx = lock_num % 32;
    uint32_t mask = 1u << bit_idx;

    return (m_locks[word_idx] & mask) != 0;
}

void LockState::notifyGUI()
{
    // Count active locks for logging
    int activeCount = 0;
    for (int i = 0; i < 256; ++i) {
        if (isLockActive(i)) {
            activeCount++;
        }
    }

    fprintf(stderr, "[LockState::notifyGUI] Lock state changed: %d locks active\n", activeCount);
    fflush(stderr);

    // Call the notification callback if set
    // The callback will create a LockStatusMessage and send it via IPC
    if (m_notifyCallback) {
        m_notifyCallback(m_locks);
    }
}

void LockState::reset()
{
    // Clear all lock bits
    for (int i = 0; i < 8; ++i) {
        m_locks[i] = 0;
    }

    fprintf(stderr, "[LockState] All locks reset to inactive\n");
    fflush(stderr);

    // Notify GUI of reset
    notifyGUI();
}

void LockState::setBit(uint8_t lock_num, bool value)
{
    uint8_t word_idx = lock_num / 32;
    uint8_t bit_idx = lock_num % 32;
    uint32_t mask = 1u << bit_idx;

    if (value) {
        m_locks[word_idx] |= mask;
    } else {
        m_locks[word_idx] &= ~mask;
    }
}

} // namespace yamy::input
