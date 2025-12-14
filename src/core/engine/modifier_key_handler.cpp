//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// modifier_key_handler.cpp - Number keys as custom hardware modifiers

#include "modifier_key_handler.h"
#include "engine_event_processor.h"
#include "../input/vk_constants.h"
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
    m_key_states[yamy_scancode].is_modal = false;

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

void ModifierKeyHandler::registerModalModifier(uint16_t yamy_scancode, int modifier_type)
{
    m_key_states[yamy_scancode] = KeyState();
    m_key_states[yamy_scancode].modal_modifier_type = modifier_type;
    m_key_states[yamy_scancode].is_modal = true;

    // Extract mod number for logging (assumes Type_Mod0 = 16, Type_Mod19 = 35)
    const int TYPE_MOD0 = 16;  // Modifier::Type_Mod0
    int mod_number = modifier_type - TYPE_MOD0;

    LOG_INFO("[ModifierKeyHandler] [MODIFIER] Registered modal modifier 0x{:04X} → mod{}",
             yamy_scancode, mod_number);
}

NumberKeyResult ModifierKeyHandler::processNumberKey(uint16_t yamy_scancode, EventType event_type)
{
    // Check if this is a registered number modifier
    auto it = m_key_states.find(yamy_scancode);
    if (it == m_key_states.end()) {
        return NumberKeyResult(ProcessingAction::NOT_A_NUMBER_MODIFIER, 0, false);
    }

    KeyState& state = it->second;
    bool is_modal = state.is_modal;
    HardwareModifier modifier = state.target_modifier;
    int modal_type = state.modal_modifier_type;

    if (event_type == EventType::PRESS) {
        // PRESS event handling
        switch (state.state) {
            case NumberKeyState::IDLE:
                // Start waiting period
                state.state = NumberKeyState::WAITING;
                state.press_time = std::chrono::steady_clock::now();

                LOG_INFO("[ModifierKeyHandler] [MODIFIER] Key 0x{:04X} PRESS, waiting for threshold ({})",
                         yamy_scancode, is_modal ? "modal" : "hardware");

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

                    if (is_modal) {
                        // Modal modifier - return modifier type, no VK code
                        LOG_INFO("[ModifierKeyHandler] [MODIFIER] Hold detected: 0x{:04X} → modal mod{} ACTIVATE",
                                 yamy_scancode, modal_type - 16);  // Type_Mod0 = 16
                        return NumberKeyResult(ProcessingAction::ACTIVATE_MODIFIER, 0, modal_type, true);
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
                // Release before threshold - TAP detected
                auto elapsed = std::chrono::steady_clock::now() - state.press_time;
                auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

                state.state = NumberKeyState::IDLE;

                LOG_INFO("[ModifierKeyHandler] [MODIFIER] Tap detected: 0x{:04X} (released after {}ms)",
                         yamy_scancode, elapsed_ms);

                // Return action to apply substitution for both PRESS and RELEASE
                // Note: The PRESS event was already consumed (WAITING_FOR_THRESHOLD)
                // So we need to output both PRESS and RELEASE for the substituted key
                return NumberKeyResult(ProcessingAction::APPLY_SUBSTITUTION_RELEASE, 0, true);
            }

            case NumberKeyState::MODIFIER_ACTIVE: {
                // Deactivate modifier
                state.state = NumberKeyState::IDLE;

                if (is_modal) {
                    // Modal modifier - return modifier type, no VK code
                    LOG_INFO("[ModifierKeyHandler] [MODIFIER] Deactivating modal: 0x{:04X} → mod{} DEACTIVATE",
                             yamy_scancode, modal_type - 16);  // Type_Mod0 = 16
                    return NumberKeyResult(ProcessingAction::DEACTIVATE_MODIFIER, 0, modal_type, true);
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
