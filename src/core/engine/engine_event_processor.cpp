//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_event_processor.cpp - Unified 3-layer event processing implementation

#include "engine_event_processor.h"
#include "../../platform/linux/keycode_mapping.h"
#include "../../utils/platform_logger.h"
#include <cstdlib>

namespace yamy {

EventProcessor::EventProcessor(const SubstitutionTable& subst_table)
    : m_substitutions(subst_table)
    , m_debugLogging(false)
{
    // Check for debug logging environment variable
    const char* debug_env = std::getenv("YAMY_DEBUG_KEYCODE");
    if (debug_env && debug_env[0] == '1') {
        m_debugLogging = true;
        PLATFORM_LOG_INFO("EventProcessor", "Debug logging enabled via YAMY_DEBUG_KEYCODE");
    }
}

EventProcessor::ProcessedEvent EventProcessor::processEvent(uint16_t input_evdev, EventType type)
{
    if (m_debugLogging) {
        const char* type_str = (type == EventType::PRESS) ? "PRESS" : "RELEASE";
        PLATFORM_LOG_DEBUG("EventProcessor", "[EVENT:START] evdev %u (%s)", input_evdev, type_str);
    }

    // Layer 1: evdev → YAMY scan code
    uint16_t yamy_l1 = layer1_evdevToYamy(input_evdev);
    if (yamy_l1 == 0) {
        if (m_debugLogging) {
            PLATFORM_LOG_DEBUG("EventProcessor", "[EVENT:END] Invalid (Layer 1 failed)");
        }
        return ProcessedEvent(0, 0, type, false);
    }

    // Layer 2: Apply substitution
    uint16_t yamy_l2 = layer2_applySubstitution(yamy_l1);

    // Layer 3: YAMY scan code → evdev
    uint16_t output_evdev = layer3_yamyToEvdev(yamy_l2);
    if (output_evdev == 0) {
        if (m_debugLogging) {
            PLATFORM_LOG_DEBUG("EventProcessor", "[EVENT:END] Invalid (Layer 3 failed)");
        }
        return ProcessedEvent(0, 0, type, false);
    }

    if (m_debugLogging) {
        const char* type_str = (type == EventType::PRESS) ? "PRESS" : "RELEASE";
        PLATFORM_LOG_DEBUG("EventProcessor", "[EVENT:END] Output evdev %u (%s)", output_evdev, type_str);
    }

    // Event type is ALWAYS preserved: PRESS in = PRESS out, RELEASE in = RELEASE out
    // Return both output evdev and output YAMY scan code (for engine compatibility)
    return ProcessedEvent(output_evdev, yamy_l2, type, true);
}

uint16_t EventProcessor::layer1_evdevToYamy(uint16_t evdev)
{
    // Call existing keycode mapping function
    // Note: event_type parameter is optional, we pass -1 to use default behavior
    uint16_t yamy = yamy::platform::evdevToYamyKeyCode(evdev, -1);

    if (m_debugLogging) {
        if (yamy != 0) {
            PLATFORM_LOG_DEBUG("EventProcessor", "[LAYER1:IN] evdev %u → yamy 0x%04X", evdev, yamy);
        } else {
            PLATFORM_LOG_DEBUG("EventProcessor", "[LAYER1:IN] evdev %u → NOT FOUND", evdev);
        }
    }

    return yamy;
}

uint16_t EventProcessor::layer2_applySubstitution(uint16_t yamy_in)
{
    // Look up in substitution table
    auto it = m_substitutions.find(yamy_in);

    if (it != m_substitutions.end()) {
        // Substitution found
        uint16_t yamy_out = it->second;
        if (m_debugLogging) {
            PLATFORM_LOG_DEBUG("EventProcessor", "[LAYER2:SUBST] 0x%04X → 0x%04X", yamy_in, yamy_out);
        }
        return yamy_out;
    } else {
        // No substitution, passthrough unchanged
        if (m_debugLogging) {
            PLATFORM_LOG_DEBUG("EventProcessor", "[LAYER2:PASSTHROUGH] 0x%04X (no substitution)", yamy_in);
        }
        return yamy_in;
    }
}

uint16_t EventProcessor::layer3_yamyToEvdev(uint16_t yamy)
{
    // Call existing keycode mapping function
    uint16_t evdev = yamy::platform::yamyToEvdevKeyCode(yamy);

    if (m_debugLogging) {
        if (evdev != 0) {
            const char* key_name = yamy::platform::getKeyName(evdev);
            PLATFORM_LOG_DEBUG("EventProcessor", "[LAYER3:OUT] yamy 0x%04X → evdev %u (%s)",
                             yamy, evdev, key_name);
        } else {
            PLATFORM_LOG_DEBUG("EventProcessor", "[LAYER3:OUT] yamy 0x%04X → NOT FOUND", yamy);
        }
    }

    return evdev;
}

} // namespace yamy
