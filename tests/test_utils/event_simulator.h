#pragma once

#include <vector>
#include <chrono>
#include <cstdint>
#include <functional>

// Forward declarations
class Engine;
class MockInputInjector;

namespace yamy::platform {
    struct KeyEvent;
    using KeyCallback = std::function<bool(const KeyEvent&)>;
}

namespace yamy {
namespace test {

/**
 * EventSimulator - Utility class for injecting events with proper timing and synchronization
 *
 * Provides infrastructure for reliable event simulation with timing control,
 * engine initialization synchronization, and output synchronization.
 *
 * Purpose: Enable reproducible test scenarios by handling timing, delays, and
 * synchronization between event injection and engine processing.
 */
class EventSimulator {
public:
    /**
     * Event structure representing a keyboard event with timing information
     */
    struct Event {
        uint16_t evdevCode;      // evdev key code (e.g., 30 for KEY_A)
        bool isKeyDown;          // true = press, false = release
        uint32_t delayMs;        // delay in milliseconds AFTER this event

        Event(uint16_t code, bool down, uint32_t delay = 0)
            : evdevCode(code), isKeyDown(down), delayMs(delay) {}
    };

    /**
     * Configuration for timing and synchronization
     */
    struct Config {
        uint32_t engineReadyTimeoutMs;  // max time to wait for engine ready (default: 5000ms)
        uint32_t outputTimeoutMs;        // max time to wait for output (default: 1000ms)
        uint32_t pollIntervalMs;         // polling interval for synchronization (default: 10ms)

        Config()
            : engineReadyTimeoutMs(5000)
            , outputTimeoutMs(1000)
            , pollIntervalMs(10) {}
    };

    /**
     * Constructor
     */
    explicit EventSimulator(const Config& config = Config());

    /**
     * Wait for Engine to be fully initialized and ready to process events
     *
     * @param engine The Engine instance to check
     * @return true if engine is ready, false if timeout
     */
    bool waitForEngineReady(Engine* engine);

    /**
     * Wait for mock injector to produce expected number of outputs
     *
     * @param mockInjector The mock injector to monitor
     * @param expectedCallCount Expected number of inject calls
     * @return true if output received, false if timeout
     */
    bool waitForOutput(MockInputInjector* mockInjector, int expectedCallCount);

    /**
     * Inject a sequence of events with proper timing
     *
     * @param keyCallback The key callback function from the input hook
     * @param events Vector of events to inject with delays
     */
    void injectSequence(yamy::platform::KeyCallback keyCallback,
                       const std::vector<Event>& events);

    /**
     * Convert YAMY scan code to evdev code
     * Common mappings:
     *   0x1e (A) -> 30
     *   0x1f (S) -> 31
     *   0x20 (D) -> 32
     *   0x23 (H) -> 35
     *   0x24 (J) -> 36
     *   0x25 (K) -> 37
     *   0x26 (L) -> 38
     *   0x27 (Semicolon) -> 39
     *   0x30 (B) -> 48
     *   0xE04B (Left) -> 105
     *   0xE050 (Down) -> 108
     *   0xE048 (Up) -> 103
     *   0xE04D (Right) -> 106
     *
     * @param yamyScanCode YAMY scan code (hex format like 0x1e)
     * @return evdev code
     */
    static uint16_t yamyToEvdev(uint16_t yamyScanCode);

private:
    Config m_config;
};

} // namespace test
} // namespace yamy
