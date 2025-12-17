//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// json_config_loader.cpp - JSON configuration loader implementation

#include "json_config_loader.h"
#include <fstream>
#include <sstream>
#include <gsl/gsl>

namespace yamy::settings {

JsonConfigLoader::JsonConfigLoader(std::ostream* log)
    : m_log(log)
    , m_keyboard(nullptr)
{
}

bool JsonConfigLoader::load(Setting* setting, const std::string& json_path)
{
    Expects(setting != nullptr);

    // TODO: Implement in task 1.9
    // - Read JSON file into string
    // - Parse with nlohmann::json with error handling
    // - Validate schema (version field required, must be "2.0")
    // - Call parseKeyboard(), parseVirtualModifiers(), parseMappings() in order

    logError("JsonConfigLoader::load() not yet implemented");
    return false;
}

bool JsonConfigLoader::parseKeyboard(const nlohmann::json& obj, Setting* setting)
{
    Expects(setting != nullptr);

    // Store keyboard pointer for later use by other methods
    m_keyboard = &setting->m_keyboard;

    // Check if keyboard section exists
    if (!obj.contains("keyboard")) {
        logError("Missing 'keyboard' section in configuration");
        return false;
    }

    const auto& keyboard = obj["keyboard"];

    // Check if keys section exists
    if (!keyboard.contains("keys")) {
        logError("Missing 'keyboard.keys' section in configuration");
        return false;
    }

    const auto& keys = keyboard["keys"];

    // Validate keys is an object
    if (!keys.is_object()) {
        logError("'keyboard.keys' must be an object");
        return false;
    }

    // Clear the lookup map before parsing
    m_keyLookup.clear();

    // Parse each key definition
    for (auto& [name, scanCodeValue] : keys.items()) {
        // Validate scan code value is a string
        if (!scanCodeValue.is_string()) {
            logError("Scan code for key '" + name + "' must be a string (e.g., \"0x1e\")");
            return false;
        }

        std::string scanCodeHex = scanCodeValue.get<std::string>();

        // Parse scan code from hex string
        uint16_t scanCode;
        if (!parseScanCode(scanCodeHex, &scanCode)) {
            logError("Invalid scan code for key '" + name + "': " + scanCodeHex);
            return false;
        }

        // Create Key object
        Key key;
        key.addName(name);
        key.addScanCode(ScanCode(scanCode, 0));  // flags = 0 for basic keys

        // Add key to keyboard
        m_keyboard->addKey(key);

        // Get pointer to the added key (it's in the keyboard's hash table now)
        Key* keyPtr = m_keyboard->searchKey(name);
        if (keyPtr == nullptr) {
            logError("Failed to add key '" + name + "' to keyboard");
            return false;
        }

        // Build lookup map for name resolution
        m_keyLookup[name] = keyPtr;
    }

    if (keys.empty()) {
        logWarning("No keys defined in 'keyboard.keys' section");
    }

    return true;
}

bool JsonConfigLoader::parseVirtualModifiers(const nlohmann::json& obj, Setting* setting)
{
    Expects(setting != nullptr);

    // virtualModifiers section is optional
    if (!obj.contains("virtualModifiers")) {
        return true;
    }

    const auto& vmods = obj["virtualModifiers"];

    // Validate virtualModifiers is an object
    if (!vmods.is_object()) {
        logError("'virtualModifiers' must be an object");
        return false;
    }

    // Parse each virtual modifier definition
    for (auto& [modName, modDef] : vmods.items()) {
        // Validate modifier name format (M00-MFF)
        if (modName.length() != 3 || modName[0] != 'M') {
            logError("Invalid virtual modifier name '" + modName + "': must be M00-MFF");
            return false;
        }

        // Extract hex number from modifier name (e.g., "M00" → 0x00, "M3A" → 0x3A)
        uint8_t modNum;
        try {
            modNum = static_cast<uint8_t>(std::stoi(modName.substr(1), nullptr, 16));
        } catch (const std::exception& e) {
            logError("Invalid modifier number in '" + modName + "': " + e.what());
            return false;
        }

        // Validate modDef is an object
        if (!modDef.is_object()) {
            logError("Virtual modifier '" + modName + "' definition must be an object");
            return false;
        }

        // Parse trigger key (required)
        if (!modDef.contains("trigger")) {
            logError("Virtual modifier '" + modName + "' missing required 'trigger' field");
            return false;
        }

        if (!modDef["trigger"].is_string()) {
            logError("Virtual modifier '" + modName + "' trigger must be a string");
            return false;
        }

        std::string triggerName = modDef["trigger"].get<std::string>();
        Key* triggerKey = resolveKeyName(triggerName);
        if (!triggerKey) {
            logError("Unknown trigger key for " + modName + ": '" + triggerName + "'");
            return false;
        }

        // Get the trigger key's scan code
        const ScanCode* scanCodes = triggerKey->getScanCodes();
        if (triggerKey->getScanCodesSize() == 0) {
            logError("Trigger key '" + triggerName + "' for " + modName + " has no scan codes");
            return false;
        }

        // Register virtual modifier trigger
        // Map: scancode → modifier number
        setting->m_virtualModTriggers[scanCodes[0].m_scan] = modNum;

        // Parse optional tap action
        if (modDef.contains("tap")) {
            if (!modDef["tap"].is_string()) {
                logError("Virtual modifier '" + modName + "' tap action must be a string");
                return false;
            }

            std::string tapName = modDef["tap"].get<std::string>();
            Key* tapKey = resolveKeyName(tapName);
            if (!tapKey) {
                logError("Unknown tap key for " + modName + ": '" + tapName + "'");
                return false;
            }

            // Get tap key's scan code
            const ScanCode* tapScanCodes = tapKey->getScanCodes();
            if (tapKey->getScanCodesSize() == 0) {
                logError("Tap key '" + tapName + "' for " + modName + " has no scan codes");
                return false;
            }

            // Register tap action
            // Map: modifier number → tap scancode
            setting->m_modTapActions[modNum] = tapScanCodes[0].m_scan;
        }

        // Note: holdThresholdMs is not stored in Setting (would need to be added)
        // For now we ignore it as it's not in the current Setting structure
    }

    if (vmods.empty()) {
        logWarning("'virtualModifiers' section is empty");
    }

    return true;
}

bool JsonConfigLoader::parseMappings(const nlohmann::json& obj, Setting* setting)
{
    Expects(setting != nullptr);

    // TODO: Implement in task 1.8
    // - Parse "mappings" array
    // - Parse "from" field using parseModifiedKey()
    // - Parse "to" field (single key or array for sequences)
    // - Create KeySeq with ActionKey objects
    // - Add mappings to global Keymap

    return false;
}

Key* JsonConfigLoader::resolveKeyName(const std::string& name)
{
    // Lookup key in the map populated by parseKeyboard()
    auto it = m_keyLookup.find(name);

    if (it != m_keyLookup.end()) {
        // Key found - return pointer
        return it->second;
    }

    // Key not found - log helpful error with suggestions
    std::ostringstream suggestions;
    suggestions << "Unknown key name '" << name << "'. ";

    // If the lookup table has keys, suggest some similar ones
    if (!m_keyLookup.empty()) {
        suggestions << "Available keys include: ";
        int count = 0;
        for (const auto& [keyName, _] : m_keyLookup) {
            if (count >= 5) break; // Limit suggestions to first 5 keys
            if (count > 0) suggestions << ", ";
            suggestions << "'" << keyName << "'";
            count++;
        }
        if (m_keyLookup.size() > 5) {
            suggestions << ", and " << (m_keyLookup.size() - 5) << " more...";
        }
    } else {
        suggestions << "No keys have been defined in 'keyboard.keys' section.";
    }

    logError(suggestions.str());
    return nullptr;
}

ModifiedKey JsonConfigLoader::parseModifiedKey(const std::string& from_spec)
{
    // TODO: Implement in task 1.7
    // - Parse modifier strings
    // - Parse standard modifiers (Shift, Ctrl, Alt, Win)
    // - Parse virtual modifiers (M00-MFF)
    // - Extract key name and resolve to Key*
    // - Build ModifiedKey object with all modifiers

    return ModifiedKey();
}

bool JsonConfigLoader::validateSchema(const nlohmann::json& config)
{
    // TODO: Implement in task 1.9
    // - Check "version" field exists and equals "2.0"
    // - Check required sections present

    return false;
}

bool JsonConfigLoader::parseScanCode(const std::string& hex_str, uint16_t* out_code)
{
    Expects(out_code != nullptr);

    // Validate format (must start with "0x" or "0X")
    if (hex_str.size() < 3 || (hex_str[0] != '0') || (hex_str[1] != 'x' && hex_str[1] != 'X')) {
        logError("Invalid scan code format '" + hex_str + "': must start with '0x'");
        return false;
    }

    // Parse hex string to uint16_t
    try {
        size_t pos;
        unsigned long value = std::stoul(hex_str, &pos, 16);

        // Check if entire string was consumed
        if (pos != hex_str.size()) {
            logError("Invalid scan code format '" + hex_str + "': contains invalid characters");
            return false;
        }

        // Check if value fits in uint16_t
        if (value > 0xFFFF) {
            logError("Invalid scan code '" + hex_str + "': value too large (max 0xFFFF)");
            return false;
        }

        *out_code = static_cast<uint16_t>(value);
        return true;
    } catch (const std::exception& e) {
        logError("Failed to parse scan code '" + hex_str + "': " + e.what());
        return false;
    }
}

void JsonConfigLoader::logError(const std::string& message)
{
    if (m_log != nullptr) {
        *m_log << "[ERROR] " << message << std::endl;
    }
}

void JsonConfigLoader::logWarning(const std::string& message)
{
    if (m_log != nullptr) {
        *m_log << "[WARNING] " << message << std::endl;
    }
}

} // namespace yamy::settings
