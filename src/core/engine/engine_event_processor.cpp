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
    std::cerr << "[LAYER3] ENTRY: yamy=0x" << std::hex << yamy << std::dec << std::endl;

    // Layer 3: YAMY → evdev conversion with virtual key suppression
    // ... (rest of the function is the same)
}

// ... other functions ...

} // namespace yamy