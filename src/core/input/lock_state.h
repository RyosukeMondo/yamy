#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// lock_state.h - Lock key state management for virtual key system
//
// Manages 256 toggleable lock keys (L00-LFF) with GUI notification support.
// Lock keys work like CapsLock: press once to activate, press again to deactivate.

#ifndef _LOCK_STATE_H
#define _LOCK_STATE_H

#include <cstdint>
#include <functional>

namespace yamy::input {

/// Callback type for GUI notification when lock state changes
/// The callback receives a pointer to the 8-element lock bits array
using LockStateChangeCallback = std::function<void(const uint32_t lockBits[8])>;

/// Lock state manager for L00-LFF virtual lock keys
/// Provides toggle functionality and GUI notifications for lock state changes
class LockState {
public:
    LockState();

    /// Toggle a lock key on/off
    /// @param lock_num Lock number (0x00-0xFF for L00-LFF)
    void toggleLock(uint8_t lock_num);

    /// Check if a lock is currently active
    /// @param lock_num Lock number (0x00-0xFF for L00-LFF)
    /// @return true if lock is active, false otherwise
    bool isLockActive(uint8_t lock_num) const;

    /// Get pointer to the complete lock bitmask array
    /// @return Pointer to 8 x uint32_t array (256 bits total) for L00-LFF
    const uint32_t* getLockBits() const { return m_locks; }

    /// Send lock status update to GUI via IPC
    /// Called automatically by toggleLock(), but can be called manually if needed
    void notifyGUI();

    /// Set callback for lock state change notifications
    /// @param callback Function to call when lock state changes (nullptr to disable)
    void setNotificationCallback(LockStateChangeCallback callback) {
        m_notifyCallback = callback;
    }

    /// Reset all lock states to inactive
    void reset();

private:
    /// Set or clear a specific lock bit
    /// @param lock_num Lock number (0x00-0xFF)
    /// @param value true to activate, false to deactivate
    void setBit(uint8_t lock_num, bool value);

    uint32_t m_locks[8];  // 256 bits for L00-LFF lock states
    LockStateChangeCallback m_notifyCallback;  // Callback for GUI notifications
};

} // namespace yamy::input

#endif // !_LOCK_STATE_H
