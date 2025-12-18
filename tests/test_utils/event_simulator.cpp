#include "event_simulator.h"
#include "../../src/core/engine/engine.h"
#include "../../src/core/engine/engine_state.h"
#include "../../src/core/platform/input_hook_interface.h"
#include "../../src/core/platform/types.h"
#include <thread>
#include <iostream>

namespace yamy {
namespace test {

EventSimulator::EventSimulator(const Config& config)
    : m_config(config) {
}

bool EventSimulator::waitForEngineReady(Engine* engine) {
    if (!engine) {
        std::cerr << "[EventSimulator] ERROR: null engine pointer" << std::endl;
        return false;
    }

    auto startTime = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(m_config.engineReadyTimeoutMs);

    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);

        if (elapsed >= timeout) {
            std::cerr << "[EventSimulator] TIMEOUT: Engine not ready after "
                      << m_config.engineReadyTimeoutMs << "ms" << std::endl;
            return false;
        }

        // Check if engine is in Running state
        yamy::EngineState state = engine->getState();
        if (state == yamy::EngineState::Running) {
            std::cout << "[EventSimulator] Engine ready after "
                      << elapsed.count() << "ms" << std::endl;
            return true;
        }

        // Sleep before next poll
        std::this_thread::sleep_for(std::chrono::milliseconds(m_config.pollIntervalMs));
    }
}

// waitForOutput is now a template in the header

void EventSimulator::injectSequence(yamy::platform::KeyCallback keyCallback,
                                   const std::vector<Event>& events) {
    if (!keyCallback) {
        std::cerr << "[EventSimulator] ERROR: null keyCallback" << std::endl;
        return;
    }

    for (const auto& event : events) {
        // Create KeyEvent structure
        yamy::platform::KeyEvent keyEvent;
        keyEvent.scanCode = event.evdevCode;
        keyEvent.isKeyDown = event.isKeyDown;
        keyEvent.isExtended = false;
        keyEvent.key = static_cast<yamy::platform::KeyCode>(0); // Will be determined by engine
        keyEvent.timestamp = 0;
        keyEvent.flags = 0;
        keyEvent.extraInfo = 0;

        std::cout << "[EventSimulator] Injecting evdev=" << event.evdevCode
                  << " isDown=" << event.isKeyDown
                  << " delay=" << event.delayMs << "ms" << std::endl;

        // Inject the event through the callback
        keyCallback(keyEvent);

        // Apply delay after event
        if (event.delayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(event.delayMs));
        }
    }
}

uint16_t EventSimulator::yamyToEvdev(uint16_t yamyScanCode) {
    // Map common YAMY scan codes to evdev codes
    switch (yamyScanCode) {
        // Standard keys
        case 0x1e: return 30;  // A
        case 0x1f: return 31;  // S
        case 0x20: return 32;  // D
        case 0x23: return 35;  // H
        case 0x24: return 36;  // J
        case 0x25: return 37;  // K
        case 0x26: return 38;  // L
        case 0x27: return 39;  // Semicolon
        case 0x30: return 48;  // B

        // Extended keys (arrow keys)
        case 0xE04B: return 105; // Left
        case 0xE050: return 108; // Down
        case 0xE048: return 103; // Up
        case 0xE04D: return 106; // Right

        // Pass through for already-converted codes or unknown codes
        default:
            // If it's already a small number, it's probably evdev
            if (yamyScanCode < 256) {
                return yamyScanCode;
            }
            // Unknown extended code
            std::cerr << "[EventSimulator] WARNING: Unknown YAMY scan code 0x"
                      << std::hex << yamyScanCode << std::dec
                      << " - returning as-is" << std::endl;
            return yamyScanCode;
    }
}

} // namespace test
} // namespace yamy
