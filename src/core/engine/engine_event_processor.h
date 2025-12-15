#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_event_processor.h - Unified 3-layer event processing architecture

#ifndef _ENGINE_EVENT_PROCESSOR_H
#define _ENGINE_EVENT_PROCESSOR_H

#include <cstdint>
#include <unordered_map>
#include <memory>
#include <functional>
#include <string>

namespace yamy {

// Forward declaration for JourneyEvent
namespace logger {
struct JourneyEvent;
}

// Forward declaration for ModifierState and LockState
namespace input {
class ModifierState;
class LockState;
}

// Forward declaration for ModifierKeyHandler
namespace engine {
class ModifierKeyHandler;
}

/// Event type enumeration for key events
enum class EventType {
    RELEASE = 0,  // Key released
    PRESS = 1,    // Key pressed
    REPEAT = 2    // Key repeat (not used in substitution)
};

/// Substitution table: maps YAMY scan code → YAMY scan code
using SubstitutionTable = std::unordered_map<uint16_t, uint16_t>;

/// Unified event processor implementing 3-layer architecture
/// Layer 1: evdev → YAMY scan code
/// Layer 2: Substitution application
/// Layer 3: YAMY scan code → evdev
class EventProcessor {
public:
    /// Result of event processing
    struct ProcessedEvent {
        uint16_t output_evdev;  ///< Output evdev code
        uint16_t output_yamy;   ///< Output YAMY scan code (after Layer 2 substitution)
        EventType type;         ///< Event type (PRESS/RELEASE)
        bool valid;             ///< false if unmapped at any layer

        ProcessedEvent()
            : output_evdev(0), output_yamy(0), type(EventType::RELEASE), valid(false) {}

        ProcessedEvent(uint16_t evdev, uint16_t yamy, EventType t, bool v)
            : output_evdev(evdev), output_yamy(yamy), type(t), valid(v) {}
    };

    /// Constructor
    /// @param subst_table Reference to substitution table (YAMY → YAMY mappings)
    explicit EventProcessor(const SubstitutionTable& subst_table);

    /// Destructor - defined in .cpp to allow forward declaration of ModifierKeyHandler
    ~EventProcessor();

    /// Main entry point: Process an input event through all 3 layers
    /// @param input_evdev Input evdev code from hardware
    /// @param type Event type (PRESS or RELEASE)
    /// @param io_modState Pointer to modifier state (input/output) - updated during processing (optional)
    /// @param io_lockState Pointer to lock state (input/output) - updated during processing (optional)
    /// @return Processed event with output evdev code and validity
    /// @note Event type is ALWAYS preserved: PRESS in = PRESS out
    ProcessedEvent processEvent(uint16_t input_evdev, EventType type, input::ModifierState* io_modState = nullptr, input::LockState* io_lockState = nullptr);

    /// Enable or disable debug logging
    /// @param enabled true to enable debug logging
    void setDebugLogging(bool enabled) { m_debugLogging = enabled; }

    /// Register a number key as a hardware modifier
    /// @param yamy_scancode YAMY scan code for number key
    /// @param modifier_yamy_code YAMY scan code for hardware modifier key
    void registerNumberModifier(uint16_t yamy_scancode, uint16_t modifier_yamy_code);

    /// Set the modifier key handler (dependency injection)
    /// @param handler Unique pointer to ModifierKeyHandler (ownership transferred)
    void setModifierHandler(std::unique_ptr<engine::ModifierKeyHandler> handler);

    /// Check if modifier handler is available
    /// @return true if handler is set, false otherwise
    bool hasModifierHandler() const { return m_modifierHandler != nullptr; }

    /// Get pointer to modifier handler (for testing/configuration)
    /// @return Pointer to handler, or nullptr if not set
    engine::ModifierKeyHandler* getModifierHandler() { return m_modifierHandler.get(); }

    /// Register virtual modifiers (M00-MFF) with tap actions
    /// @param mod_tap_actions Map of modifier number (0x00-0xFF) to tap output keycode
    void registerVirtualModifiers(const std::unordered_map<uint8_t, uint16_t>& mod_tap_actions);

    /// Callback type for journey event notifications
    /// Called after each event is processed with the journey data
    using JourneyEventCallback = std::function<void(const logger::JourneyEvent&)>;

    /// Set callback for journey event notifications
    /// @param callback Function to call with journey events (or nullptr to disable)
    void setJourneyEventCallback(JourneyEventCallback callback) {
        m_journeyCallback = callback;
    }

private:
    /// Layer 1: Map evdev code to YAMY scan code
    /// @param evdev Input evdev code
    /// @return YAMY scan code, or 0 if unmapped
    /// @note Logs: [LAYER1:IN] evdev X → yamy 0xYYYY
    uint16_t layer1_evdevToYamy(uint16_t evdev);

    /// Layer 2: Apply substitution from .mayu configuration
    /// @param yamy_in Input YAMY scan code
    /// @param type Event type (PRESS or RELEASE) - needed for number modifier processing
    /// @param io_modState Pointer to modifier state (input/output) - updated for modal modifiers
    /// @param io_lockState Pointer to lock state (input/output) - updated for lock keys (optional)
    /// @return Substituted YAMY scan code, or input unchanged if no substitution
    /// @note Logs: [LAYER2:SUBST] or [LAYER2:PASSTHROUGH]
    /// @note Pure function: NO special cases for any key type
    /// @note Number modifiers checked BEFORE substitution lookup
    uint16_t layer2_applySubstitution(uint16_t yamy_in, EventType type, input::ModifierState* io_modState, input::LockState* io_lockState);

    /// Layer 3: Map YAMY scan code to output evdev code
    /// @param yamy Input YAMY scan code
    /// @return Output evdev code, or 0 if unmapped
    /// @note Logs: [LAYER3:OUT] yamy 0xYYYY → evdev Z
    uint16_t layer3_yamyToEvdev(uint16_t yamy);

    const SubstitutionTable& m_substitutions;       ///< Reference to substitution table
    bool m_debugLogging;                            ///< Debug logging enabled flag
    std::unique_ptr<engine::ModifierKeyHandler> m_modifierHandler;  ///< Number modifier handler
    JourneyEventCallback m_journeyCallback;         ///< Callback for journey event notifications
};

} // namespace yamy

#endif // _ENGINE_EVENT_PROCESSOR_H
