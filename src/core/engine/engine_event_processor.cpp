//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_event_processor.cpp - Unified 3-layer event processing implementation

#include "engine_event_processor.h"
#include "modifier_key_handler.h"
#include "../input/modifier_state.h"
#include "../input/keyboard.h"
#include "../../platform/linux/keycode_mapping.h"
#include "../../utils/logger.h"
#include "../logger/journey_logger.h"
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <iomanip>

namespace yamy {

EventProcessor::EventProcessor(const SubstitutionTable& subst_table)
    : m_substitutions(subst_table)
    , m_debugLogging(false)
    , m_modifierHandler(std::make_unique<engine::ModifierKeyHandler>())
    , m_currentEventIsTap(false)
    , m_lookupTable(std::make_unique<engine::RuleLookupTable>())
{
    // Check for debug logging environment variable
    const char* debug_env = std::getenv("YAMY_DEBUG_KEYCODE");
    if (debug_env && debug_env[0] == '1') {
        m_debugLogging = true;
        LOG_INFO("[EventProcessor] Debug logging enabled via YAMY_DEBUG_KEYCODE");
    }

    // Number modifiers will be registered dynamically from .mayu configuration
    // via registerNumberModifier() called from Engine::buildSubstitutionTable()
}

EventProcessor::~EventProcessor() = default;

EventProcessor::ProcessedEvent EventProcessor::processEvent(uint16_t input_evdev, EventType type, input::ModifierState* io_modState)
{
    // Reset TAP flag for this event
    m_currentEventIsTap = false;

    // Check all WAITING virtual modifiers and activate those that exceeded threshold
    // This ensures that if a modifier key is held while another key is pressed,
    // the modifier is activated BEFORE we process the new key event
    if (m_modifierHandler && io_modState) {
        auto to_activate = m_modifierHandler->checkAndActivateWaitingModifiers();
        for (const auto& [scancode, mod_num] : to_activate) {
            io_modState->activateModifier(mod_num);
        }
    }

    // Create journey event for tracking (if console logging OR investigate window is active)
    yamy::logger::JourneyEvent journey;
    if (yamy::logger::JourneyLogger::isEnabled() || m_journeyCallback) {
        journey.start_time = std::chrono::steady_clock::now();
        journey.evdev_input = input_evdev;
        journey.is_key_down = (type == EventType::PRESS);
        journey.device_event_number = -1; // TODO: pass from caller if needed
        journey.input_key_name = yamy::platform::getKeyName(input_evdev);
    }

    if (m_debugLogging) {
        const char* type_str = (type == EventType::PRESS) ? "PRESS" : "RELEASE";
        LOG_DEBUG("[EventProcessor] [EVENT:START] evdev {} ({})", input_evdev, type_str);
    }

    // Layer 1: evdev → YAMY scan code
    uint16_t yamy_l1 = layer1_evdevToYamy(input_evdev);
    if (yamy_l1 == 0) {
        if (m_debugLogging) {
            LOG_DEBUG("[EventProcessor] [EVENT:END] Invalid (Layer 1 failed)");
        }
        return ProcessedEvent(0, 0, type, false);
    }

    if (yamy::logger::JourneyLogger::isEnabled() || m_journeyCallback) {
        journey.yamy_input = yamy_l1;
    }

    // Layer 2: Apply substitution (with number modifier and lock support)
    uint16_t yamy_l2 = layer2_applySubstitution(yamy_l1, type, io_modState);

    if (yamy::logger::JourneyLogger::isEnabled() || m_journeyCallback) {
        journey.yamy_output = yamy_l2;
        journey.was_substituted = (yamy_l1 != yamy_l2);
        // Number modifier info will be filled by layer2 if applicable
    }

    // Layer 3: YAMY scan code → evdev
    uint16_t output_evdev = layer3_yamyToEvdev(yamy_l2);
    if (output_evdev == 0) {
        if (m_debugLogging) {
            LOG_DEBUG("[EventProcessor] [EVENT:END] Invalid (Layer 3 failed)");
        }
        return ProcessedEvent(0, 0, type, false);
    }

    if (yamy::logger::JourneyLogger::isEnabled() || m_journeyCallback) {
        journey.evdev_output = output_evdev;
        journey.output_key_name = yamy::platform::getKeyName(output_evdev);
        journey.end_time = std::chrono::steady_clock::now();
        journey.latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            journey.end_time - journey.start_time).count();
        journey.valid = true;

        // Log the complete journey to stdout (if enabled)
        if (yamy::logger::JourneyLogger::isEnabled()) {
            yamy::logger::JourneyLogger::logJourney(journey);
        }

        // Notify callback (for Investigate Window)
        if (m_journeyCallback) {
            m_journeyCallback(journey);
        }
    }

    if (m_debugLogging) {
        const char* type_str = (type == EventType::PRESS) ? "PRESS" : "RELEASE";
        LOG_DEBUG("[EventProcessor] [EVENT:END] Output evdev {} ({})", output_evdev, type_str);
    }

    // Event type is ALWAYS preserved: PRESS in = PRESS out, RELEASE in = RELEASE out
    // Return both output evdev and output YAMY scan code (for engine compatibility)
    // Pass is_tap flag to indicate this TAP event needs PRESS+RELEASE output
    return ProcessedEvent(output_evdev, yamy_l2, type, true, m_currentEventIsTap);
}

uint16_t EventProcessor::layer1_evdevToYamy(uint16_t evdev)
{
    // Call existing keycode mapping function
    // Note: event_type parameter is optional, we pass -1 to use default behavior
    uint16_t yamy = yamy::platform::evdevToYamyKeyCode(evdev, -1);

    if (m_debugLogging) {
        if (yamy != 0) {
            LOG_DEBUG("[EventProcessor] [LAYER1:IN] evdev {} → yamy 0x{:04X}", evdev, yamy);
        } else {
            LOG_DEBUG("[EventProcessor] [LAYER1:IN] evdev {} → NOT FOUND", evdev);
        }
    }

    return yamy;
}

uint16_t EventProcessor::layer2_applySubstitution(uint16_t yamy_in, EventType type, input::ModifierState* io_modState)
{
    // Layer 2a: Substitution Table Lookup
    //
    // CRITICAL REFACTOR: Apply substitution FIRST, then check if result is a virtual modifier
    // Old approach was wrong: it checked if INPUT is a modifier, but substitution hadn't happened yet
    // New approach: Do substitution, then check if RESULT needs tap/hold processing
    //
    // Flow:
    // 1. Apply substitution: yamy_in → yamy_out
    // 2. If yamy_out is a virtual modifier (M00-MFF) → tap/hold detection
    // 3. If yamy_out is a lock key (L00-LFF) → toggle on PRESS, suppress always
    // 4. Otherwise → return yamy_out for output

    // Step 1: Apply substitution using the new RuleLookupTable
    uint16_t yamy_out = yamy_in;
    bool mapped = false;

    if (io_modState && m_lookupTable) {
        const auto& state = io_modState->getFullState();
        if (const auto* match = m_lookupTable->findMatch(yamy_in, state)) {
            yamy_out = match->outputScanCode;
            mapped = true;

            if (m_debugLogging) {
                LOG_DEBUG("[EventProcessor] [LAYER2:MATCH] 0x{:04X} -> 0x{:04X} (New Lookup)",
                          yamy_in, yamy_out);
            }
        }
    }

    if (!mapped) {
        // If no rule matched, check for simple substitutions (e.g., for virtual modifier triggers)
        auto simple_it = m_substitutions.find(yamy_in);
        if (simple_it != m_substitutions.end()) {
             yamy_out = simple_it->second;
        }
    }

    // Step 2: Check if result (yamy_out) is a virtual modifier that needs tap/hold detection
    // Use range check for virtual modifiers (0xF000-0xF0FF) since we register the PHYSICAL key, not the virtual code
    if (m_modifierHandler && yamy::platform::isModifier(yamy_out)) {
        engine::NumberKeyResult result = m_modifierHandler->processNumberKey(yamy_in, type);

        switch (result.action) {
            case engine::ProcessingAction::ACTIVATE_MODIFIER:
                // HOLD detected - activate modifier
                if (m_modifierHandler->isVirtualModifier(yamy_in) && io_modState) {
                    // Virtual modifier (M00-MFF) - update ModifierState using new method
                    uint8_t mod_num = static_cast<uint8_t>(result.modifier_type);
                    io_modState->activateModifier(mod_num);
                    if (m_debugLogging) {
                        LOG_DEBUG("[EventProcessor] [LAYER2:VIRTUAL_MOD] M{:02X} (0x{:04X}) HOLD → ACTIVATE",
                                  mod_num, yamy_in);
                    }
                    return 0;  // Suppress event (no output)
                } else {
                    // Hardware modifier - return VK code for injection
                    if (m_debugLogging) {
                        LOG_DEBUG("[EventProcessor] [LAYER2:NUMBER_MOD] 0x{:04X} HOLD → modifier VK 0x{:04X}",
                                  yamy_in, result.output_yamy_code);
                    }
                    return result.output_yamy_code;
                }

            case engine::ProcessingAction::DEACTIVATE_MODIFIER:
                // Modifier release
                if (m_modifierHandler->isVirtualModifier(yamy_in) && io_modState) {
                    // Virtual modifier (M00-MFF) - update ModifierState using new method
                    uint8_t mod_num = static_cast<uint8_t>(result.modifier_type);
                    io_modState->deactivateModifier(mod_num);
                    if (m_debugLogging) {
                        LOG_DEBUG("[EventProcessor] [LAYER2:VIRTUAL_MOD] M{:02X} (0x{:04X}) RELEASE → DEACTIVATE",
                                  mod_num, yamy_in);
                    }
                    return 0;  // Suppress event (no output)
                } else {
                    // Hardware modifier - return VK code for injection
                    if (m_debugLogging) {
                        LOG_DEBUG("[EventProcessor] [LAYER2:NUMBER_MOD] 0x{:04X} RELEASE modifier VK 0x{:04X}",
                                  yamy_in, result.output_yamy_code);
                    }
                    return result.output_yamy_code;
                }

            case engine::ProcessingAction::APPLY_SUBSTITUTION_PRESS:
            case engine::ProcessingAction::APPLY_SUBSTITUTION_RELEASE:
                // TAP detected
                // Since we're already in the virtual modifier branch (yamy_out is 0xF000-0xF0FF),
                // we know this is a virtual modifier and should return the tap output
                if (result.output_yamy_code != 0) {
                    std::cerr << "[LAYER2:TAP] Virtual modifier TAP detected! yamy_in=0x" << std::hex << yamy_in
                              << ", tap_output=0x" << result.output_yamy_code << std::dec
                              << ", action=" << (result.action == engine::ProcessingAction::APPLY_SUBSTITUTION_RELEASE ? "RELEASE" : "PRESS")
                              << std::endl;

                    // If TAP detected on RELEASE, set flag to indicate PRESS+RELEASE needed
                    if (result.action == engine::ProcessingAction::APPLY_SUBSTITUTION_RELEASE) {
                        m_currentEventIsTap = true;
                        std::cerr << "[LAYER2:TAP] Set m_currentEventIsTap=true for TAP on RELEASE" << std::endl;
                    }

                    if (m_debugLogging) {
                        LOG_DEBUG("[EventProcessor] [LAYER2:VIRTUAL_MOD] 0x{:04X} TAP detected → output 0x{:04X}",
                                  yamy_in, result.output_yamy_code);
                    }
                    return result.output_yamy_code;
                } else {
                    // Number/modal modifier TAP - fall through to normal substitution logic
                    std::cerr << "[LAYER2:TAP] TAP detected but no tap output, yamy_in=0x" << std::hex << yamy_in << std::dec << std::endl;
                    if (m_debugLogging) {
                        LOG_DEBUG("[EventProcessor] [LAYER2:NUMBER_MOD] 0x{:04X} TAP detected, applying substitution",
                                  yamy_in);
                    }
                    break;
                }

            case engine::ProcessingAction::WAITING_FOR_THRESHOLD:
                // Still waiting for hold threshold - suppress this event
                // The event will be re-evaluated on RELEASE or threshold expiry
                if (m_debugLogging) {
                    LOG_DEBUG("[EventProcessor] [LAYER2:NUMBER_MOD] 0x{:04X} waiting for threshold, suppressing",
                              yamy_in);
                }
                return 0;  // Signal suppression (will fail at Layer 3)

            case engine::ProcessingAction::NOT_A_NUMBER_MODIFIER:
            default:
                // Not a number modifier, proceed with normal substitution
                break;
        }
    }

    // Step 3: Check if result (yamy_out) is a lock key (L00-LFF)
    // Lock keys toggle their state on PRESS and are always suppressed (never output to system)
    if (yamy::platform::isLock(yamy_out) && io_modState) {
        if (type == EventType::PRESS) {
            // Toggle lock on PRESS
            uint8_t lock_num = static_cast<uint8_t>(yamy_out - 0xF100);  // Extract L00-LFF number
            io_modState->toggleLock(lock_num);

            std::cerr << "[LAYER2:LOCK] L" << std::hex << (int)lock_num << " (0x" << yamy_out << std::dec
                      << ") PRESS → toggle" << std::endl;
            if (m_debugLogging) {
                bool is_active = io_modState->isLockActive(lock_num);
                LOG_DEBUG("[EventProcessor] [LAYER2:LOCK] L{:02X} (0x{:04X}) PRESS → toggle to {}",
                          lock_num, yamy_out, is_active ? "ACTIVE" : "INACTIVE");
            }
        } else {
            // RELEASE - just suppress, no toggle
            uint8_t lock_num = static_cast<uint8_t>(yamy_out - 0xF100);
            std::cerr << "[LAYER2:LOCK] L" << std::hex << (int)lock_num << " (0x" << yamy_out << std::dec
                      << ") RELEASE → suppress" << std::endl;
            if (m_debugLogging) {
                LOG_DEBUG("[EventProcessor] [LAYER2:LOCK] L{:02X} (0x{:04X}) RELEASE → suppress",
                          lock_num, yamy_out);
            }
        }
        return 0;  // Suppress event (lock keys never output to system)
    }

    // Step 4: Return the substituted (or original) yamy code for output
    std::cerr << "[LAYER2] Final output: 0x" << std::hex << yamy_out << std::dec << std::endl;
    return yamy_out;
}

uint16_t EventProcessor::layer3_yamyToEvdev(uint16_t yamy)
{
    std::cerr << "[LAYER3] ENTRY: yamy=0x" << std::hex << yamy << std::dec << std::endl;

    // Layer 3: YAMY → evdev conversion with virtual key suppression
    // ... (rest of the function is the same)
}

// ... other functions ...

} // namespace yamy