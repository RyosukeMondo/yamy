#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace yamy {
namespace logger {

/**
 * @brief Hardware device information for journey logging
 */
struct DeviceInfo {
    std::string path;           // /dev/input/eventX
    std::string name;           // Device name (e.g., "USB Keyboard")
    uint16_t vendor_id;         // USB vendor ID
    uint16_t product_id;        // USB product ID
    std::string serial;         // Serial number (if available)
    int event_number;           // Event number (e.g., 3 for event3)

    DeviceInfo()
        : vendor_id(0), product_id(0), event_number(-1) {}
};

/**
 * @brief Complete journey data for a single key event
 */
struct JourneyEvent {
    // Layer 0: Input capture
    int device_event_number;        // Which /dev/input/eventX
    uint16_t evdev_input;          // Raw evdev code from hardware
    std::string input_key_name;    // Human-readable name (e.g., "A")

    // Layer 1: YAMY scan code conversion
    uint16_t yamy_input;           // YAMY scan code after layer 1

    // Layer 2: Substitution/transformation
    uint16_t yamy_output;          // YAMY code after substitution
    std::string output_key_name;   // Human-readable output name (e.g., "J")
    bool was_substituted;          // True if substitution was applied
    bool was_number_modifier;      // True if number modifier logic applied
    std::string modifier_action;   // "HOLD", "TAP", "WAIT", or empty

    // Layer 3: Output conversion
    uint16_t evdev_output;         // Final evdev code for injection

    // Timing
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    uint64_t latency_ns;           // End-to-end latency in nanoseconds

    // Event metadata
    bool is_key_down;              // true = press, false = release
    bool valid;                    // True if event was processed successfully

    JourneyEvent()
        : device_event_number(-1)
        , evdev_input(0)
        , yamy_input(0)
        , yamy_output(0)
        , was_substituted(false)
        , was_number_modifier(false)
        , evdev_output(0)
        , latency_ns(0)
        , is_key_down(false)
        , valid(false)
    {}
};

/**
 * @brief Journey logger for tracing key event transformations
 */
class JourneyLogger {
public:
    /**
     * @brief Initialize the journey logger
     * @param enable Enable journey logging
     * @param use_color Use ANSI color codes
     * @param compact_mode Skip passthrough (unchanged) events
     */
    static void initialize(bool enable = false, bool use_color = false, bool compact_mode = false);

    /**
     * @brief Check if journey logging is enabled
     */
    static bool isEnabled();

    /**
     * @brief Print the legend/header (call once at startup)
     * @param devices List of hardware devices being monitored
     */
    static void printLegend(const std::vector<DeviceInfo>& devices);

    /**
     * @brief Log a complete key event journey
     * @param event The journey event data
     */
    static void logJourney(const JourneyEvent& event);

    /**
     * @brief Format a key name with proper padding
     * @param key_name Raw key name
     * @param width Target width (default 5)
     * @return Padded key name
     */
    static std::string formatKeyName(const std::string& key_name, int width = 5);

    /**
     * @brief Get a short device identifier string
     * @param event_number Event number (e.g., 3 for event3)
     * @return Formatted device ID (e.g., "[ev3]")
     */
    static std::string getDeviceId(int event_number);

private:
    static bool s_enabled;
    static bool s_use_color;
    static bool s_compact_mode;
    static bool s_legend_printed;

    // ANSI color codes
    static const char* COLOR_RESET;
    static const char* COLOR_GREEN;   // Normal remapping
    static const char* COLOR_YELLOW;  // Number modifier TAP
    static const char* COLOR_CYAN;    // Number modifier HOLD
    static const char* COLOR_RED;     // Failed/dropped events
    static const char* COLOR_GRAY;    // Passthrough

    /**
     * @brief Format a single journey line
     */
    static std::string formatJourneyLine(const JourneyEvent& event);

    /**
     * @brief Get color code based on event type
     */
    static const char* getEventColor(const JourneyEvent& event);
};

} // namespace logger
} // namespace yamy
