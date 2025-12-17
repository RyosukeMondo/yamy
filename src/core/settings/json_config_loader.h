#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// json_config_loader.h - JSON configuration loader for YAMY

#ifndef _JSON_CONFIG_LOADER_H
#define _JSON_CONFIG_LOADER_H

#include <string>
#include <ostream>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "setting.h"
#include "../input/keyboard.h"

namespace yamy::settings {

/**
 * @brief JSON configuration file loader
 *
 * Loads YAMY configuration from JSON files, replacing the legacy .mayu parser.
 * Supports:
 * - Keyboard key definitions (scan code mappings)
 * - M00-MFF virtual modifiers with tap actions
 * - Key mappings (from → to rules)
 * - Key sequences (output multiple keys)
 *
 * Design follows requirement FR-1 for JSON configuration format.
 */
class JsonConfigLoader {
public:
    /**
     * @brief Construct loader with optional logging stream
     * @param log Output stream for warnings/errors (nullptr = no logging)
     */
    explicit JsonConfigLoader(std::ostream* log = nullptr);

    /**
     * @brief Load JSON configuration file
     * @param setting Output setting object (must not be nullptr)
     * @param json_path Path to JSON config file
     * @return true on success, false on error (see log for details)
     *
     * @pre setting != nullptr
     * @pre json_path is valid file path
     * @post setting populated with keyboard, keymaps, and virtual modifiers
     */
    bool load(Setting* setting, const std::string& json_path);

private:
    /**
     * @brief Parse keyboard.keys section
     * @param obj JSON object containing keyboard definition
     * @param setting Setting object to populate
     * @return true on success, false on error
     *
     * Parses key definitions with scan codes:
     * "keys": { "A": "0x1e", "CapsLock": "0x3a" }
     */
    bool parseKeyboard(const nlohmann::json& obj, Setting* setting);

    /**
     * @brief Parse virtualModifiers section
     * @param obj JSON object containing virtual modifier definitions
     * @param setting Setting object to populate
     * @return true on success, false on error
     *
     * Parses M00-MFF virtual modifiers with trigger keys and tap actions:
     * "M00": { "trigger": "CapsLock", "tap": "Escape", "holdThresholdMs": 200 }
     */
    bool parseVirtualModifiers(const nlohmann::json& obj, Setting* setting);

    /**
     * @brief Parse mappings array
     * @param obj JSON object containing mapping definitions
     * @param setting Setting object to populate
     * @return true on success, false on error
     *
     * Parses key mappings:
     * { "from": "M00-A", "to": "Left" }
     * { "from": "M00-B", "to": ["Escape", "B"] }
     */
    bool parseMappings(const nlohmann::json& obj, Setting* setting);

    /**
     * @brief Resolve key name to Key pointer
     * @param name Key name (e.g., "A", "CapsLock", "Left")
     * @return Pointer to Key object, or nullptr if not found
     *
     * Looks up key in m_keyLookup map populated by parseKeyboard().
     * Logs helpful error with suggestions if key not found.
     */
    Key* resolveKeyName(const std::string& name);

    /**
     * @brief Parse modified key expression
     * @param from_spec Key expression (e.g., "Shift-M00-A")
     * @return ModifiedKey with modifiers and key
     *
     * Parses expressions like:
     * - "A" → ModifiedKey(no modifiers, A)
     * - "Shift-A" → ModifiedKey(Shift, A)
     * - "M00-A" → ModifiedKey(M00, A)
     * - "Shift-M00-A" → ModifiedKey(Shift+M00, A)
     */
    ModifiedKey parseModifiedKey(const std::string& from_spec);

    /**
     * @brief Validate JSON schema version
     * @param config JSON configuration object
     * @return true if valid, false otherwise
     *
     * Validates:
     * - "version" field exists and equals "2.0"
     * - Required sections present
     */
    bool validateSchema(const nlohmann::json& config);

    /**
     * @brief Parse scan code hex string
     * @param hex_str Hex string (e.g., "0x1e", "0x3a")
     * @param out_code Output scan code
     * @return true if valid, false otherwise
     */
    bool parseScanCode(const std::string& hex_str, uint16_t* out_code);

    /**
     * @brief Log error message
     * @param message Error message
     */
    void logError(const std::string& message);

    /**
     * @brief Log warning message
     * @param message Warning message
     */
    void logWarning(const std::string& message);

    // State
    std::ostream* m_log;                              ///< Optional logging stream
    std::unordered_map<std::string, Key*> m_keyLookup; ///< Key name → Key* lookup
    Keyboard* m_keyboard;                             ///< Current keyboard (set during load)
};

} // namespace yamy::settings

#endif // !_JSON_CONFIG_LOADER_H
