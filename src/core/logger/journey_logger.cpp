#include "journey_logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>

namespace yamy {
namespace logger {

// Static member initialization
bool JourneyLogger::s_enabled = false;
bool JourneyLogger::s_use_color = false;
bool JourneyLogger::s_compact_mode = false;
bool JourneyLogger::s_legend_printed = false;

// ANSI color codes
const char* JourneyLogger::COLOR_RESET = "\033[0m";
const char* JourneyLogger::COLOR_GREEN = "\033[32m";
const char* JourneyLogger::COLOR_YELLOW = "\033[33m";
const char* JourneyLogger::COLOR_CYAN = "\033[36m";
const char* JourneyLogger::COLOR_RED = "\033[31m";
const char* JourneyLogger::COLOR_GRAY = "\033[90m";

void JourneyLogger::initialize(bool enable, bool use_color, bool compact_mode) {
    s_enabled = enable;
    s_use_color = use_color;
    s_compact_mode = compact_mode;
    s_legend_printed = false;

    // Check environment variables
    const char* env_enabled = std::getenv("YAMY_JOURNEY_LOG");
    if (env_enabled && std::string(env_enabled) == "1") {
        s_enabled = true;
    }

    const char* env_color = std::getenv("YAMY_JOURNEY_COLOR");
    if (env_color && std::string(env_color) == "1") {
        s_use_color = true;
    }

    const char* env_compact = std::getenv("YAMY_JOURNEY_COMPACT");
    if (env_compact && std::string(env_compact) == "1") {
        s_compact_mode = true;
    }
}

bool JourneyLogger::isEnabled() {
    return s_enabled;
}

void JourneyLogger::printLegend(const std::vector<DeviceInfo>& devices) {
    if (!s_enabled || s_legend_printed) {
        return;
    }

    std::cout << "╔═══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                        YAMY KEY EVENT JOURNEY LOG                              ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ Format: [Dev] (evdev|Key) (YAMY_IN|Key)->(YAMY_OUT|Key)->(Output Latency) ↓↑ ║\n";
    std::cout << "║                                                                                ║\n";
    std::cout << "║ Columns:                                                                       ║\n";
    std::cout << "║   [Dev]     - Device ID (e.g., ev3 for /dev/input/event3)                    ║\n";
    std::cout << "║   evdev     - Linux input event code from hardware                            ║\n";
    std::cout << "║   YAMY_IN   - YAMY internal scan code (after layer 1 conversion)             ║\n";
    std::cout << "║   YAMY_OUT  - After substitution/number modifier (layer 2)                   ║\n";
    std::cout << "║   Output    - Final evdev code injected to system (layer 3)                  ║\n";
    std::cout << "║   Latency   - End-to-end processing time (nanoseconds)                        ║\n";
    std::cout << "║   ↓↑        - Direction: ↓=Pressed, ↑=Released                               ║\n";
    std::cout << "║                                                                                ║\n";

    if (!devices.empty()) {
        std::cout << "║ Hardware Devices:                                                              ║\n";
        for (const auto& dev : devices) {
            std::ostringstream line;
            line << "║   [ev" << dev.event_number << "] " << dev.name;
            if (dev.vendor_id != 0 || dev.product_id != 0) {
                line << " (" << std::hex << std::setw(4) << std::setfill('0')
                     << dev.vendor_id << ":" << std::setw(4) << dev.product_id << ")";
            }
            if (!dev.serial.empty()) {
                line << " S/N:" << dev.serial;
            }

            // Pad to 80 characters
            std::string line_str = line.str();
            while (line_str.length() < 80) {
                line_str += " ";
            }
            line_str += "║";
            std::cout << line_str << "\n";
        }
    }

    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << std::flush;

    s_legend_printed = true;
}

void JourneyLogger::logJourney(const JourneyEvent& event) {
    if (!s_enabled) {
        return;
    }

    // Skip passthrough events in compact mode
    if (s_compact_mode && !event.was_substituted && !event.was_number_modifier) {
        return;
    }

    // Skip invalid events
    if (!event.valid) {
        return;
    }

    std::string line = formatJourneyLine(event);

    if (s_use_color) {
        const char* color = getEventColor(event);
        std::cout << color << line << COLOR_RESET << "\n";
    } else {
        std::cout << line << "\n";
    }

    std::cout << std::flush;
}

std::string JourneyLogger::formatKeyName(const std::string& key_name, int width) {
    if (key_name.empty()) {
        return std::string(width, ' ');
    }

    std::string result = key_name;

    // Truncate if too long
    if (result.length() > static_cast<size_t>(width)) {
        result = result.substr(0, width);
    }

    // Pad if too short
    while (result.length() < static_cast<size_t>(width)) {
        result += " ";
    }

    return result;
}

std::string JourneyLogger::getDeviceId(int event_number) {
    if (event_number < 0) {
        return "[???]";
    }

    std::ostringstream oss;
    oss << "[ev" << event_number << "]";
    return oss.str();
}

std::string JourneyLogger::formatJourneyLine(const JourneyEvent& event) {
    std::ostringstream line;

    // Device ID: [ev3] (5 chars)
    line << std::left << std::setw(5) << getDeviceId(event.device_event_number) << " ";

    // Layer 0: (evdev|Key) - (3|5 = 9 chars + parens = 11 chars)
    line << "(" << std::right << std::setw(3) << event.evdev_input << "|"
         << std::left << formatKeyName(event.input_key_name, 5) << ") ";

    // Layer 1: (YAMY_IN|Key) - (6|5 = 14 chars)
    line << "(0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
         << event.yamy_input << std::dec << "|"
         << std::left << formatKeyName(event.input_key_name, 5) << ")";

    // Arrow or passthrough indicator
    if (event.was_substituted || event.was_number_modifier) {
        line << "->";
    } else {
        line << "══";  // Passthrough indicator
    }

    // Layer 2: (YAMY_OUT|Key) - (6|5 = 14 chars)
    line << "(0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
         << event.yamy_output << std::dec << "|"
         << std::left << formatKeyName(event.output_key_name, 5) << ")";

    // Layer 3: ->(Output Latency)
    line << "->(";

    // Output evdev code (3 chars)
    line << std::right << std::setw(3) << event.evdev_output << "|";

    // Output key name (8 chars)
    line << std::left << formatKeyName(event.output_key_name, 8);

    // Latency (7 chars: " 150ns")
    line << std::right << std::setw(4) << (event.latency_ns) << "ns";

    line << ") ";

    // Direction indicator (1 char)
    line << (event.is_key_down ? "↓" : "↑") << " ";

    // Action (8 chars)
    std::string action = event.is_key_down ? "Pressed " : "Released";
    line << std::left << std::setw(8) << action;

    // Optional: Number modifier info
    if (event.was_number_modifier && !event.modifier_action.empty()) {
        line << " [" << event.modifier_action << "]";
    }

    return line.str();
}

const char* JourneyLogger::getEventColor(const JourneyEvent& event) {
    if (!s_use_color) {
        return "";
    }

    // Failed/invalid events
    if (!event.valid) {
        return COLOR_RED;
    }

    // Number modifier HOLD
    if (event.was_number_modifier && event.modifier_action == "HOLD") {
        return COLOR_CYAN;
    }

    // Number modifier TAP
    if (event.was_number_modifier && event.modifier_action == "TAP") {
        return COLOR_YELLOW;
    }

    // Normal substitution
    if (event.was_substituted) {
        return COLOR_GREEN;
    }

    // Passthrough (no change)
    return COLOR_GRAY;
}

} // namespace logger
} // namespace yamy
