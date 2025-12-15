//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// modifier_key_handler.cpp - Number keys as custom hardware modifiers

#include "modifier_key_handler.h"
#include "engine_event_processor.h"
#include "../input/vk_constants.h"
#include "../input/keyboard.h"
#include "../../utils/logger.h"
#include <chrono>

namespace yamy {
namespace engine {

ModifierKeyHandler::ModifierKeyHandler(uint32_t hold_threshold_ms)
    : m_hold_threshold_ms(hold_threshold_ms)
{
    LOG_INFO("[ModifierKeyHandler] [MODIFIER] initialized with threshold {}ms", hold_threshold_ms);
}

void ModifierKeyHandler::registerNumberModifier(uint16_t yamy_scancode, HardwareModifier modifier)
{
    m_number_to_modifier[yamy_scancode] = modifier;
    m_key_states[yamy_scancode] = KeyState();
    m_key_states[yamy_scancode].target_modifier = modifier;

    LOG_INFO("[ModifierKeyHandler] [MODIFIER] Registered number key 0x{:04X} → {}",
             yamy_scancode,
             modifier == HardwareModifier::LSHIFT ? "LSHIFT" :
             modifier == HardwareModifier::RSHIFT ? "RSHIFT" :
             modifier == HardwareModifier::LCTRL ? "LCTRL" :
             modifier == HardwareModifier::RCTRL ? "RCTRL" :
             modifier == HardwareModifier::LALT ? "LALT" :
             modifier == HardwareModifier::RALT ? "RALT" :
             modifier == HardwareModifier::LWIN ? "LWIN" :
             modifier == HardwareModifier::RWIN ? "RWIN" : "NONE");
}

void ModifierKeyHandler::registerVirtualModifier(uint16_t modifier_code, uint16_t tap_output)
{
    // Store in virtual modifiers map
    m_virtual_modifiers[modifier_code] = tap_output;

    // Create key state for this virtual modifier
    KeyState& state = m_key_states[modifier_code];
    state.is_virtual = true;
    state.virtual_mod_num = static_cast<uint8_t>(modifier_code - 0xF000);  // Extract M00-MFF number
    state.tap_output = tap_output;

    LOG_INFO("[ModifierKeyHandler] [MODIFIER] Registered virtual modifier M{:02X} (0x{:04X}), tap_output=0x{:04X}",
             state.virtual_mod_num, modifier_code, tap_output);
}

void ModifierKeyHandler::registerVirtualModifiersFromMap(const std::unordered_map<uint8_t, uint16_t>& mod_tap_actions)
{
    for (const auto& [mod_num, tap_output] : mod_tap_actions) {
        uint16_t modifier_code = 0xF000 + mod_num;  // Convert M00-MFF number to keycode
        registerVirtualModifier(modifier_code, tap_output);
    }
    LOG_INFO("[ModifierKeyHandler] [MODIFIER] Registered {} virtual modifiers from map", mod_tap_actions.size());
}

void ModifierKeyHandler::registerVirtualModifierTrigger(uint16_t trigger_key, uint8_t mod_num, uint16_t tap_output)
{
    // Register the PHYSICAL KEY (trigger_key) as the key to track in m_key_states
    // When this key is pressed, it will activate the virtual modifier M<mod_num>

    KeyState& state = m_key_states[trigger_key];  // Use trigger_key, not modifier code!
    state.is_virtual = true;
    state.virtual_mod_num = mod_num;
    state.tap_output = tap_output;
    state.state = NumberKeyState::IDLE;

    fprintf(stderr, "[MODIFIER] Registered virtual modifier trigger: physical key 0x%04X → M%02X, tap_output=0x%04X\n",
            trigger_key, mod_num, tap_output);
    LOG_INFO("[ModifierKeyHandler] [MODIFIER] Registered virtual modifier M{:02X}: trigger=0x{:04X}, tap_output=0x{:04X}",
             mod_num, trigger_key, tap_output);
}

NumberKeyResult ModifierKeyHandler::processNumberKey(uint16_t yamy_scancode, EventType event_type)
{
    // Check if this is a registered number modifier
    auto it = m_key_states.find(yamy_scancode);
    if (it == m_key_states.end()) {
        return NumberKeyResult(ProcessingAction::NOT_A_NUMBER_MODIFIER, 0, false);
    }

    KeyState& state = it->second;
    bool is_virtual = state.is_virtual;
    HardwareModifier modifier = state.target_modifier;
    uint8_t virtual_mod_num = state.virtual_mod_num;
    uint16_t tap_output = state.tap_output;

    if (event_type == EventType::PRESS) {
        // PRESS event handling
        switch (state.state) {
            case NumberKeyState::IDLE:
                // Start waiting period
                state.state = NumberKeyState::WAITING;
                state.press_time = std::chrono::steady_clock::now();

                LOG_INFO("[ModifierKeyHandler] [MODIFIER] Key 0x{:04X} PRESS, waiting for threshold ({})",
                         yamy_scancode, is_virtual ? "virtual" : "hardware");

                return NumberKeyResult(ProcessingAction::WAITING_FOR_THRESHOLD, 0, false);

            case NumberKeyState::WAITING:
                // Check if threshold exceeded (happens on next event or repeated PRESS)
                if (hasExceededMaximum(state.press_time)) {
                    // System suspend/resume - reset to IDLE
                    LOG_WARN("[ModifierKeyHandler] [MODIFIER] Maximum exceeded, resetting to IDLE");
                    state.state = NumberKeyState::IDLE;
                    return NumberKeyResult(ProcessingAction::NOT_A_NUMBER_MODIFIER, 0, false);
                }

                if (hasExceededThreshold(state.press_time)) {
                    // Hold detected - activate modifier
                    state.state = NumberKeyState::MODIFIER_ACTIVE;

                    if (is_virtual) {
                        // Virtual modifier (M00-MFF) - activate in ModifierState, no VK code
                        LOG_INFO("[ModifierKeyHandler] [MODIFIER] Hold detected: M{:02X} (0x{:04X}) ACTIVATE",
                                 virtual_mod_num, yamy_scancode);
                        return NumberKeyResult(ProcessingAction::ACTIVATE_MODIFIER, 0, virtual_mod_num, true);
                    } else {
                        // Hardware modifier - return VK code
                        uint16_t vk_code = getModifierVKCode(modifier);
                        LOG_INFO("[ModifierKeyHandler] [MODIFIER] Hold detected: 0x{:04X} → modifier VK 0x{:04X} PRESS",
                                 yamy_scancode, vk_code);
                        return NumberKeyResult(ProcessingAction::ACTIVATE_MODIFIER, vk_code, true);
                    }
                }

                // Still waiting
                return NumberKeyResult(ProcessingAction::WAITING_FOR_THRESHOLD, 0, false);

            case NumberKeyState::MODIFIER_ACTIVE:
                // Already active, ignore repeated PRESS
                LOG_INFO("[ModifierKeyHandler] [MODIFIER] Number key 0x{:04X} already active, ignoring PRESS",
                         yamy_scancode);
                return NumberKeyResult(ProcessingAction::WAITING_FOR_THRESHOLD, 0, false);

            case NumberKeyState::TAP_DETECTED:
                // This shouldn't happen (TAP transitions back to IDLE on RELEASE)
                // Treat as new PRESS
                state.state = NumberKeyState::WAITING;
                state.press_time = std::chrono::steady_clock::now();
                return NumberKeyResult(ProcessingAction::WAITING_FOR_THRESHOLD, 0, false);
        }
    }
    else if (event_type == EventType::RELEASE) {
        // RELEASE event handling
        switch (state.state) {
            case NumberKeyState::IDLE:
                // Spurious RELEASE without PRESS - graceful degradation
                LOG_WARN("[ModifierKeyHandler] [MODIFIER] RELEASE without PRESS for 0x{:04X}",
                         yamy_scancode);
                return NumberKeyResult(ProcessingAction::NOT_A_NUMBER_MODIFIER, 0, false);

            case NumberKeyState::WAITING: {
                // Check if threshold was exceeded during the hold
                auto elapsed = std::chrono::steady_clock::now() - state.press_time;
                auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

                // If threshold exceeded, treat as HOLD (even though we're at RELEASE now)
                // This is a fallback case - normally checkAndActivateWaitingModifiers() would have
                // activated the modifier already. This case only happens if no other events occurred.
                if (hasExceededThreshold(state.press_time)) {
                    // HOLD was active but we missed the activation (no other event triggered the check)
                    // Just suppress the key - don't try to deactivate since we never activated
                    state.state = NumberKeyState::IDLE;

                    if (is_virtual) {
                        LOG_INFO("[ModifierKeyHandler] [MODIFIER] HOLD detected on RELEASE (fallback): M{:02X} (held {}ms) → suppress",
                                 virtual_mod_num, elapsed_ms);
                        // Suppress the key (no tap output)
                        return NumberKeyResult(ProcessingAction::WAITING_FOR_THRESHOLD, 0, false);
                    } else {
                        LOG_INFO("[ModifierKeyHandler] [MODIFIER] HOLD detected on RELEASE (fallback): 0x{:04X} (held {}ms) → suppress",
                                 yamy_scancode, elapsed_ms);
                        return NumberKeyResult(ProcessingAction::WAITING_FOR_THRESHOLD, 0, false);
                    }
                }

                // Release before threshold - TAP detected
                state.state = NumberKeyState::IDLE;

                // For virtual modifiers, check if tap output is defined
                if (is_virtual) {
                    if (tap_output != 0) {
                        LOG_INFO("[ModifierKeyHandler] [MODIFIER] Tap detected: M{:02X} (released after {}ms) → output 0x{:04X}",
                                 virtual_mod_num, elapsed_ms, tap_output);
                        // Return tap output keycode
                        return NumberKeyResult(ProcessingAction::APPLY_SUBSTITUTION_RELEASE, tap_output, true);
                    } else {
                        LOG_INFO("[ModifierKeyHandler] [MODIFIER] Tap detected: M{:02X} (released after {}ms), no tap output defined",
                                 virtual_mod_num, elapsed_ms);
                        // No tap output, suppress
                        return NumberKeyResult(ProcessingAction::WAITING_FOR_THRESHOLD, 0, false);
                    }
                } else {
                    LOG_INFO("[ModifierKeyHandler] [MODIFIER] Tap detected: 0x{:04X} (released after {}ms)",
                             yamy_scancode, elapsed_ms);
                    // Return action to apply substitution for both PRESS and RELEASE
                    // Note: The PRESS event was already consumed (WAITING_FOR_THRESHOLD)
                    // So we need to output both PRESS and RELEASE for the substituted key
                    return NumberKeyResult(ProcessingAction::APPLY_SUBSTITUTION_RELEASE, 0, true);
                }
            }

            case NumberKeyState::MODIFIER_ACTIVE: {
                // Deactivate modifier
                state.state = NumberKeyState::IDLE;

                if (is_virtual) {
                    // Virtual modifier (M00-MFF) - deactivate in ModifierState
                    LOG_INFO("[ModifierKeyHandler] [MODIFIER] Deactivating virtual: M{:02X} (0x{:04X}) DEACTIVATE",
                             virtual_mod_num, yamy_scancode);
                    return NumberKeyResult(ProcessingAction::DEACTIVATE_MODIFIER, 0, virtual_mod_num, true);
                } else {
                    // Hardware modifier - return VK code
                    uint16_t vk_code = getModifierVKCode(modifier);
                    LOG_INFO("[ModifierKeyHandler] [MODIFIER] Deactivating modifier: 0x{:04X} → VK 0x{:04X} RELEASE",
                             yamy_scancode, vk_code);
                    return NumberKeyResult(ProcessingAction::DEACTIVATE_MODIFIER, vk_code, true);
                }
            }

            case NumberKeyState::TAP_DETECTED:
                // Already transitioned to IDLE, this is the RELEASE event
                state.state = NumberKeyState::IDLE;
                return NumberKeyResult(ProcessingAction::APPLY_SUBSTITUTION_RELEASE, 0, true);
        }
    }

    // Should not reach here
    return NumberKeyResult(ProcessingAction::NOT_A_NUMBER_MODIFIER, 0, false);
}

bool ModifierKeyHandler::isNumberModifier(uint16_t yamy_scancode) const
{
    return m_number_to_modifier.find(yamy_scancode) != m_number_to_modifier.end();
}

bool ModifierKeyHandler::isVirtualModifier(uint16_t yamy_code) const
{
    auto it = m_key_states.find(yamy_code);
    return (it != m_key_states.end()) && it->second.is_virtual;
}

bool ModifierKeyHandler::isModifierHeld(uint16_t yamy_scancode) const
{
    auto it = m_key_states.find(yamy_scancode);
    if (it == m_key_states.end()) {
        return false;
    }
    return it->second.state == NumberKeyState::MODIFIER_ACTIVE;
}

void ModifierKeyHandler::reset()
{
    for (auto& pair : m_key_states) {
        pair.second.state = NumberKeyState::IDLE;
    }
    LOG_INFO("[ModifierKeyHandler] [MODIFIER] All number key states reset to IDLE");
}

const std::unordered_map<uint16_t, ModifierKeyHandler::KeyState>& ModifierKeyHandler::getKeyStates() const
{
    return m_key_states;
}

bool ModifierKeyHandler::isWaitingForThreshold(uint16_t yama_scancode) const
{
    auto it = m_key_states.find(yama_scancode);
    if (it == m_key_states.end()) {
        return false;
    }
    return it->second.state == NumberKeyState::WAITING;
}

std::vector<std::pair<uint16_t, uint8_t>> ModifierKeyHandler::checkAndActivateWaitingModifiers()
{
    std::vector<std::pair<uint16_t, uint8_t>> to_activate;

    for (auto& [scancode, state] : m_key_states) {
        // Only check WAITING modifiers
        if (state.state != NumberKeyState::WAITING) {
            continue;
        }

        // Check if threshold exceeded
        if (hasExceededThreshold(state.press_time)) {
            // Activate this modifier
            state.state = NumberKeyState::MODIFIER_ACTIVE;

            if (state.is_virtual) {
                LOG_INFO("[ModifierKeyHandler] [MODIFIER] Auto-activating M{:02X} (0x{:04X}) - threshold exceeded",
                         state.virtual_mod_num, scancode);
                to_activate.push_back({scancode, state.virtual_mod_num});
            } else {
                LOG_INFO("[ModifierKeyHandler] [MODIFIER] Auto-activating hardware modifier (0x{:04X}) - threshold exceeded",
                         scancode);
                // For hardware modifiers, we'd need to inject the modifier key
                // For now, just log - we can extend this later if needed
            }
        }
    }

    return to_activate;
}

uint16_t ModifierKeyHandler::getModifierVKCode(HardwareModifier modifier)
{
    switch (modifier) {
        case HardwareModifier::LSHIFT:  return VK_LSHIFT;   // 0xA0
        case HardwareModifier::RSHIFT:  return VK_RSHIFT;   // 0xA1
        case HardwareModifier::LCTRL:   return VK_LCONTROL; // 0xA2
        case HardwareModifier::RCTRL:   return VK_RCONTROL; // 0xA3
        case HardwareModifier::LALT:    return VK_LMENU;    // 0xA4
        case HardwareModifier::RALT:    return VK_RMENU;    // 0xA5
        case HardwareModifier::LWIN:    return VK_LWIN;     // 0x5B
        case HardwareModifier::RWIN:    return VK_RWIN;     // 0x5C
        case HardwareModifier::NONE:
        default:
            return 0;
    }
}

bool ModifierKeyHandler::hasExceededThreshold(const std::chrono::steady_clock::time_point& press_time) const
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - press_time;
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    return elapsed_ms >= m_hold_threshold_ms;
}

bool ModifierKeyHandler::hasExceededMaximum(const std::chrono::steady_clock::time_point& press_time) const
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - press_time;
    auto elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    // Maximum threshold: 5 seconds (handles system suspend/resume edge case)
    return elapsed_sec > 5;
}

} // namespace engine
} // namespace yamy
