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

namespace yamy {

EventProcessor::EventProcessor(const SubstitutionTable& subst_table)
    : m_substitutions(subst_table)
    , m_debugLogging(false)
    , m_modifierHandler(std::make_unique<engine::ModifierKeyHandler>())
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

    // Layer 2: Apply substitution (with number modifier support)
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
    return ProcessedEvent(output_evdev, yamy_l2, type, true);
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
    // The substitution table maps physical keys to either:
    //   - Other physical keys (e.g., *A = *B)
    //   - Virtual keys (e.g., *A = *V_B, *B = *M00, *CapsLock = *L00)
    //
    // Virtual keys defined:
    //   V_*: Virtual regular keys (0xE000-0xEFFF) - intermediate mappings
    //   M00-MFF: Modal modifiers (0xF000-0xF0FF) - 256 user-defined modifiers
    //   L00-LFF: Lock keys (0xF100-0xF1FF) - 256 toggleable locks
    //
    // Virtual keys are suppressed at Layer 3 (never output to evdev).

    // CRITICAL: Check if key is registered as number, modal, OR virtual modifier BEFORE substitution lookup
    // This ensures number keys can act as modifiers (HOLD) or be substituted (TAP)
    // modal modifiers (!! operator) can activate modal modifier state
    // and virtual modifiers (M00-MFF) support tap/hold detection
    if (m_modifierHandler && (m_modifierHandler->isNumberModifier(yamy_in) ||
                               m_modifierHandler->isModalModifier(yamy_in) ||
                               m_modifierHandler->isVirtualModifier(yamy_in))) {
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
                } else if (result.modifier_type >= 0 && io_modState) {
                    // Modal modifier - update ModifierState only
                    io_modState->activate(static_cast<Modifier::Type>(result.modifier_type));
                    if (m_debugLogging) {
                        LOG_DEBUG("[EventProcessor] [LAYER2:MODAL_MOD] 0x{:04X} HOLD → mod{} ACTIVATE",
                                  yamy_in, result.modifier_type - 16);  // Type_Mod0 = 16
                    }
                    return 0;  // Suppress event (no VK code to inject)
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
                } else if (result.modifier_type >= 0 && io_modState) {
                    // Modal modifier - update ModifierState only
                    io_modState->deactivate(static_cast<Modifier::Type>(result.modifier_type));
                    if (m_debugLogging) {
                        LOG_DEBUG("[EventProcessor] [LAYER2:MODAL_MOD] 0x{:04X} RELEASE → mod{} DEACTIVATE",
                                  yamy_in, result.modifier_type - 16);  // Type_Mod0 = 16
                    }
                    return 0;  // Suppress event (no VK code to inject)
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
                if (m_modifierHandler->isVirtualModifier(yamy_in) && result.output_yamy_code != 0) {
                    // Virtual modifier with tap output defined - use tap output keycode
                    if (m_debugLogging) {
                        LOG_DEBUG("[EventProcessor] [LAYER2:VIRTUAL_MOD] 0x{:04X} TAP detected → output 0x{:04X}",
                                  yamy_in, result.output_yamy_code);
                    }
                    return result.output_yamy_code;
                } else {
                    // Number/modal modifier TAP - fall through to normal substitution logic
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

    // Normal substitution lookup (for non-number keys or TAP-detected number keys)
    auto it = m_substitutions.find(yamy_in);

    if (it != m_substitutions.end()) {
        // Substitution found
        uint16_t yamy_out = it->second;
        if (m_debugLogging) {
            LOG_DEBUG("[EventProcessor] [LAYER2:SUBST] 0x{:04X} → 0x{:04X}", yamy_in, yamy_out);
        }
        return yamy_out;
    } else {
        // No substitution, passthrough unchanged
        if (m_debugLogging) {
            LOG_DEBUG("[EventProcessor] [LAYER2:PASSTHROUGH] 0x{:04X} (no substitution)", yamy_in);
        }
        return yamy_in;
    }
}

uint16_t EventProcessor::layer3_yamyToEvdev(uint16_t yamy)
{
    // Layer 3: YAMY → evdev conversion with virtual key suppression
    //
    // Virtual keys (V_*, M00-MFF, L00-LFF) have no evdev codes and must not
    // be output to the system. They are used internally for:
    //   - V_*: Intermediate key mappings in substitution layer
    //   - M00-MFF: Modal modifier state (processed separately)
    //   - L00-LFF: Lock key state (processed separately)
    //
    // Suppress virtual keys by returning 0 (which will fail at Layer 3).
    if (yamy::platform::isVirtualKey(yamy) ||
        yamy::platform::isModifier(yamy) ||
        yamy::platform::isLock(yamy)) {
        if (m_debugLogging) {
            LOG_DEBUG("[EventProcessor] [LAYER3:SUPPRESS] yamy 0x{:04X} (virtual key, not output)", yamy);
        }
        return 0;  // Suppress virtual keys
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

void EventProcessor::registerNumberModifier(uint16_t yamy_scancode, uint16_t modifier_yamy_code)
{
    // Convert modifier YAMY scan code to evdev code to determine HardwareModifier enum
    uint16_t modifier_evdev = yamy::platform::yamyToEvdevKeyCode(modifier_yamy_code);

    if (modifier_evdev == 0) {
        LOG_WARN("[EventProcessor] Cannot map modifier YAMY code 0x{:04X} to evdev",
                 modifier_yamy_code);
        return;
    }

    // Map evdev codes to HardwareModifier enum
    // KEY_LEFTSHIFT (42), KEY_RIGHTSHIFT (54)
    // KEY_LEFTCTRL (29), KEY_RIGHTCTRL (97)
    // KEY_LEFTALT (56), KEY_RIGHTALT (100)
    // KEY_LEFTMETA (125), KEY_RIGHTMETA (126)
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
            LOG_WARN("[EventProcessor] Unknown modifier evdev code {} for number key 0x{:04X}",
                     modifier_evdev, yamy_scancode);
            return;
    }

    m_modifierHandler->registerNumberModifier(yamy_scancode, hw_mod);
    LOG_INFO("[EventProcessor] Registered number modifier: 0x{:04X} → 0x{:04X} (evdev {})",
             yamy_scancode, modifier_yamy_code, modifier_evdev);
}

} // namespace yamy
