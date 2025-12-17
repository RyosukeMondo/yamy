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

EventProcessor::EventProcessor()
    : m_debugLogging(false)
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
    // Step 1: Apply substitution using the new RuleLookupTable
    if (io_modState && m_lookupTable) {
        const auto& state = io_modState->getFullState();
        if (const auto* match = m_lookupTable->findMatch(yamy_in, state)) {
            if (m_debugLogging) {
                LOG_DEBUG("[EventProcessor] [LAYER2:MATCH] 0x{:04X} -> 0x{:04X} (New Lookup)",
                          yamy_in, match->outputScanCode);
            }
            return match->outputScanCode;
        }
    }

    // If no rule matched, pass-through
    return yamy_in;
}

uint16_t EventProcessor::layer3_yamyToEvdev(uint16_t yamy)
{
    if (m_debugLogging) {
        LOG_DEBUG("[EventProcessor] [LAYER3:IN] Processing yamy 0x{:04X}", yamy);
    }
    // Layer 3: YAMY → evdev conversion with virtual key suppression
    // Virtual keys (V_*, M00-MFF, L00-LFF) have no evdev codes and must NOT
    // be output to the system.
    bool is_virtual = yamy::platform::isVirtualKey(yamy);
    bool is_modifier = yamy::platform::isModifier(yamy);
    bool is_lock = yamy::platform::isLock(yamy);

    if (is_virtual || is_modifier || is_lock) {
        if (m_debugLogging) {
            LOG_DEBUG("[EventProcessor] [LAYER3:SUPPRESS] yamy 0x{:04X} (virtual key, not output)", yamy);
        }
        return 0;  // Suppress virtual keys (no evdev output)
    }

    // Call existing keycode mapping function for physical keys
    uint16_t evdev = yamy::platform::yamyToEvdevKeyCode(yamy);

    if (m_debugLogging) {
        if (evdev != 0) {
            const char* key_name = yamy::platform::getKeyName(evdev);
            LOG_DEBUG("[EventProcessor] [LAYER3:OUT] yamy 0x{:04X} → evdev {} ({})",
                      yamy, evdev, key_name);
        } else {
            LOG_DEBUG("[EventProcessor] [LAYER3:OUT] yamy 0x{:04X} → NOT FOUND", yamy);
        }
    }

    return evdev;
}

// ... other functions ...

void EventProcessor::setModifierHandler(std::unique_ptr<engine::ModifierKeyHandler> handler)
{
    m_modifierHandler = std::move(handler);
}

void EventProcessor::registerVirtualModifiers(const std::unordered_map<uint8_t, uint16_t>& mod_tap_actions)
{
    if (m_modifierHandler) {
        m_modifierHandler->registerVirtualModifiersFromMap(mod_tap_actions);
    }
}

void EventProcessor::registerVirtualModifierTrigger(uint16_t trigger_key, uint8_t mod_num, uint16_t tap_output)
{
    if (m_modifierHandler) {
        m_modifierHandler->registerVirtualModifierTrigger(trigger_key, mod_num, tap_output);
    }
}

void EventProcessor::registerNumberModifier(uint16_t yamy_scancode, uint16_t modifier_yamy_code)
{
    // Convert modifier YAMY scan code to evdev code to determine HardwareModifier enum
    uint16_t modifier_evdev = yamy::platform::yamyToEvdevKeyCode(modifier_yamy_code);

    if (modifier_evdev == 0) {
        return;
    }

    // Map evdev codes to HardwareModifier enum
    engine::HardwareModifier hw_mod = engine::HardwareModifier::NONE;
    switch (modifier_evdev) {
        case 42:  hw_mod = engine::HardwareModifier::LSHIFT; break;
        case 54:  hw_mod = engine::HardwareModifier::RSHIFT; break;
        case 29:  hw_mod = engine::HardwareModifier::LCTRL; break;
        case 97:  hw_mod = engine::HardwareModifier::RCTRL; break;
        case 56:  hw_mod = engine::HardwareModifier::LALT; break;
        case 100: hw_mod = engine::HardwareModifier::RALT; break;
        case 125: hw_mod = engine::HardwareModifier::LWIN; break;
        case 126: hw_mod = engine::HardwareModifier::RWIN; break;
        default:
            return;
    }

    if (m_modifierHandler) {
        m_modifierHandler->registerNumberModifier(yamy_scancode, hw_mod);
    }
}

} // namespace yamy