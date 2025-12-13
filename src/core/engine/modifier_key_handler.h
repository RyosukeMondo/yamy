#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// modifier_key_handler.h - Number keys as custom hardware modifiers
//
// Implements hold-vs-tap detection for number keys:
// - HOLD (≥200ms): Activate hardware modifier (LShift, RCtrl, etc.)
// - TAP (<200ms): Apply normal substitution
//
// Design: Passive timestamp-based detection (no timer threads)
// Integration: Layer 2 of EventProcessor (before substitution lookup)

#ifndef _MODIFIER_KEY_HANDLER_H
#define _MODIFIER_KEY_HANDLER_H

#include <cstdint>
#include <unordered_map>
#include <chrono>

namespace yamy {

// Forward declaration
enum class EventType;

namespace engine {

/// Hardware modifier types (aligned with VK constants)
enum class HardwareModifier : uint8_t {
    NONE = 0,
    LSHIFT,   // VK_LSHIFT (0xA0)
    RSHIFT,   // VK_RSHIFT (0xA1)
    LCTRL,    // VK_LCONTROL (0xA2)
    RCTRL,    // VK_RCONTROL (0xA3)
    LALT,     // VK_LMENU (0xA4)
    RALT,     // VK_RMENU (0xA5)
    LWIN,     // VK_LWIN (0x5B)
    RWIN      // VK_RWIN (0x5C)
};

/// Number key state for hold-vs-tap detection
enum class NumberKeyState : uint8_t {
    IDLE,              // Not pressed
    WAITING,           // Pressed, timer running, waiting for threshold
    MODIFIER_ACTIVE,   // Hold detected (≥threshold), modifier activated
    TAP_DETECTED       // Release before threshold, treat as tap
};

/// Processing action returned by processNumberKey()
enum class ProcessingAction : uint8_t {
    NOT_A_NUMBER_MODIFIER,      // Key is not registered as number modifier
    ACTIVATE_MODIFIER,          // HOLD detected, activate hardware modifier
    DEACTIVATE_MODIFIER,        // RELEASE after HOLD, deactivate modifier
    APPLY_SUBSTITUTION_PRESS,   // TAP detected on PRESS, apply substitution
    APPLY_SUBSTITUTION_RELEASE, // TAP detected on RELEASE, apply substitution
    WAITING_FOR_THRESHOLD       // Still waiting for hold threshold
};

/// Result from processNumberKey()
struct NumberKeyResult {
    ProcessingAction action;
    uint16_t output_yamy_code;  // Hardware modifier VK code if ACTIVATE/DEACTIVATE
    bool valid;

    NumberKeyResult()
        : action(ProcessingAction::NOT_A_NUMBER_MODIFIER)
        , output_yamy_code(0)
        , valid(false) {}

    NumberKeyResult(ProcessingAction a, uint16_t code, bool v)
        : action(a), output_yamy_code(code), valid(v) {}
};

/// Handler for number keys as custom hardware modifiers
/// Implements hold-vs-tap detection with configurable threshold (default 200ms)
///
/// Thread safety: Not thread-safe - designed to be owned by EventProcessor
/// which processes events single-threaded.
class ModifierKeyHandler {
public:
    /// Constructor
    /// @param hold_threshold_ms Hold detection threshold in milliseconds (default: 200)
    explicit ModifierKeyHandler(uint32_t hold_threshold_ms = 200);

    /// Register a number key as a hardware modifier
    /// @param yamy_scancode YAMY scan code for number key (e.g., 0x0002 for _1)
    /// @param modifier Hardware modifier to activate (e.g., LSHIFT)
    void registerNumberModifier(uint16_t yamy_scancode, HardwareModifier modifier);

    /// Process a number key event (PRESS or RELEASE)
    /// Implements state machine for hold-vs-tap detection
    /// @param yamy_scancode YAMY scan code of the key
    /// @param event_type PRESS or RELEASE
    /// @return Processing result indicating action to take
    NumberKeyResult processNumberKey(uint16_t yamy_scancode, EventType event_type);

    /// Check if a YAMY scan code is registered as a number modifier
    /// @param yamy_scancode YAMY scan code to check
    /// @return true if registered, false otherwise
    bool isNumberModifier(uint16_t yamy_scancode) const;

    /// Check if a number modifier is currently held (MODIFIER_ACTIVE state)
    /// @param yamy_scancode YAMY scan code to check
    /// @return true if held, false otherwise
    bool isModifierHeld(uint16_t yamy_scancode) const;

    /// Reset all number key states (for testing or recovery)
    void reset();

private:
    /// Mapping: YAMY number key scan code → hardware modifier type
    std::unordered_map<uint16_t, HardwareModifier> m_number_to_modifier;

    /// Per-key state tracking
    struct KeyState {
        NumberKeyState state;
        std::chrono::steady_clock::time_point press_time;
        HardwareModifier target_modifier;

        KeyState()
            : state(NumberKeyState::IDLE)
            , press_time()
            , target_modifier(HardwareModifier::NONE) {}
    };

    /// State for each registered number key
    std::unordered_map<uint16_t, KeyState> m_key_states;

    /// Hold threshold in milliseconds
    uint32_t m_hold_threshold_ms;

    /// Get hardware modifier VK code for a given modifier type
    /// @param modifier Hardware modifier type
    /// @return VK code (e.g., VK_LSHIFT = 0xA0)
    static uint16_t getModifierVKCode(HardwareModifier modifier);

    /// Check if hold threshold has been exceeded
    /// @param press_time Timestamp when key was pressed
    /// @return true if elapsed time >= threshold, false otherwise
    bool hasExceededThreshold(const std::chrono::steady_clock::time_point& press_time) const;

    /// Check if elapsed time exceeds maximum (for handling system suspend/resume)
    /// @param press_time Timestamp when key was pressed
    /// @return true if elapsed time > 5 seconds (treat as IDLE)
    bool hasExceededMaximum(const std::chrono::steady_clock::time_point& press_time) const;
};

} // namespace engine
} // namespace yamy

#endif // _MODIFIER_KEY_HANDLER_H
