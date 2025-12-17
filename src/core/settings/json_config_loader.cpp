//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// json_config_loader.cpp - JSON configuration loader implementation

#include "json_config_loader.h"
#include <fstream>
#include <sstream>
#include <vector>
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

    // Read JSON file into string
    std::ifstream file(json_path);
    if (!file.is_open()) {
        logError("Failed to open configuration file: " + json_path);
        return false;
    }

    // Read entire file into string
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string jsonContent = buffer.str();
    file.close();

    if (jsonContent.empty()) {
        logError("Configuration file is empty: " + json_path);
        return false;
    }

    // Parse JSON with error handling
    nlohmann::json config;
    try {
        config = nlohmann::json::parse(jsonContent);
    } catch (const nlohmann::json::parse_error& e) {
        std::ostringstream error;
        error << "JSON parse error in " << json_path << " at byte " << e.byte
              << ": " << e.what();
        logError(error.str());
        return false;
    } catch (const std::exception& e) {
        logError("Failed to parse JSON from " + json_path + ": " + e.what());
        return false;
    }

    // Validate schema
    if (!validateSchema(config)) {
        logError("Schema validation failed for " + json_path);
        return false;
    }

    // Parse keyboard definitions (required section)
    if (!parseKeyboard(config, setting)) {
        logError("Failed to parse keyboard section in " + json_path);
        return false;
    }

    // Parse virtual modifiers (optional section)
    if (!parseVirtualModifiers(config, setting)) {
        logError("Failed to parse virtualModifiers section in " + json_path);
        return false;
    }

    // Parse mappings (optional section)
    if (!parseMappings(config, setting)) {
        logError("Failed to parse mappings section in " + json_path);
        return false;
    }

    // Success!
    return true;
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

    // mappings section is optional
    if (!obj.contains("mappings")) {
        return true;
    }

    const auto& mappings = obj["mappings"];

    // Validate mappings is an array
    if (!mappings.is_array()) {
        logError("'mappings' must be an array");
        return false;
    }

    // Get or create the global keymap
    Keymap* globalKeymap = setting->m_keymaps.searchByName("Global");
    if (!globalKeymap) {
        // Create the global keymap if it doesn't exist
        Keymap newKeymap(
            "Global",
            nullptr,  // no default keyseq
            nullptr   // no parent keymap
        );
        globalKeymap = setting->m_keymaps.add(newKeymap);
        if (!globalKeymap) {
            logError("Failed to create global keymap");
            return false;
        }
    }

    // Parse each mapping definition
    int mappingIndex = 0;
    for (const auto& mapping : mappings) {
        mappingIndex++;

        // Validate mapping is an object
        if (!mapping.is_object()) {
            logError("Mapping #" + std::to_string(mappingIndex) + " must be an object");
            return false;
        }

        // Parse "from" field (required)
        if (!mapping.contains("from")) {
            logError("Mapping #" + std::to_string(mappingIndex) + " missing required 'from' field");
            return false;
        }

        if (!mapping["from"].is_string()) {
            logError("Mapping #" + std::to_string(mappingIndex) + " 'from' field must be a string");
            return false;
        }

        std::string fromSpec = mapping["from"].get<std::string>();
        ModifiedKey fromKey = parseModifiedKey(fromSpec);
        if (!fromKey.m_key) {
            logError("Failed to parse 'from' key in mapping #" + std::to_string(mappingIndex) + ": '" + fromSpec + "'");
            return false;
        }

        // Parse "to" field (required, can be string or array)
        if (!mapping.contains("to")) {
            logError("Mapping #" + std::to_string(mappingIndex) + " missing required 'to' field");
            return false;
        }

        const auto& toField = mapping["to"];

        // Create a KeySeq to hold the action(s)
        // Use a unique name for each mapping keyseq
        std::string keySeqName = "mapping_" + std::to_string(mappingIndex) + "_" + fromSpec;
        KeySeq keySeq(keySeqName);
        keySeq.setMode(Modifier::Type_ASSIGN);

        // Check if "to" is a string (single key) or array (sequence)
        if (toField.is_string()) {
            // Single key mapping
            std::string toSpec = toField.get<std::string>();
            ModifiedKey toKey = parseModifiedKey(toSpec);
            if (!toKey.m_key) {
                logError("Failed to parse 'to' key in mapping #" + std::to_string(mappingIndex) + ": '" + toSpec + "'");
                return false;
            }

            // Add ActionKey to the KeySeq
            ActionKey actionKey(toKey);
            keySeq.add(actionKey);
        }
        else if (toField.is_array()) {
            // Key sequence mapping
            if (toField.empty()) {
                logError("Mapping #" + std::to_string(mappingIndex) + " 'to' array is empty");
                return false;
            }

            for (size_t i = 0; i < toField.size(); ++i) {
                if (!toField[i].is_string()) {
                    logError("Mapping #" + std::to_string(mappingIndex) + " 'to' array element " +
                             std::to_string(i) + " must be a string");
                    return false;
                }

                std::string toSpec = toField[i].get<std::string>();
                ModifiedKey toKey = parseModifiedKey(toSpec);
                if (!toKey.m_key) {
                    logError("Failed to parse 'to' key in mapping #" + std::to_string(mappingIndex) +
                             " sequence element " + std::to_string(i) + ": '" + toSpec + "'");
                    return false;
                }

                // Add ActionKey to the KeySeq
                ActionKey actionKey(toKey);
                keySeq.add(actionKey);
            }

            // Mark as KEYSEQ mode for sequences
            keySeq.setMode(Modifier::Type_KEYSEQ);
        }
        else {
            logError("Mapping #" + std::to_string(mappingIndex) + " 'to' field must be a string or array");
            return false;
        }

        // Add the KeySeq to Setting's KeySeqs collection
        KeySeq* addedKeySeq = setting->m_keySeqs.add(keySeq);
        if (!addedKeySeq) {
            logError("Failed to add keyseq for mapping #" + std::to_string(mappingIndex));
            return false;
        }

        // Add the mapping to the global keymap
        globalKeymap->addAssignment(fromKey, addedKeySeq);
    }

    if (mappings.empty()) {
        logWarning("'mappings' array is empty");
    }

    return true;
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
    // Split by '-' delimiter to extract modifiers and key name
    std::vector<std::string> parts;
    std::istringstream stream(from_spec);
    std::string part;

    while (std::getline(stream, part, '-')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }

    if (parts.empty()) {
        logError("Empty key specification");
        return ModifiedKey();
    }

    // Last part is the key name, everything before is modifiers
    std::string keyName = parts.back();

    // Resolve key name to Key*
    Key* key = resolveKeyName(keyName);
    if (!key) {
        // Error already logged by resolveKeyName()
        return ModifiedKey();
    }

    // Create ModifiedKey with the resolved key
    ModifiedKey mkey(key);

    // Parse all modifiers (all parts except the last one)
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        const std::string& mod = parts[i];

        // Check if it's a standard modifier
        if (mod == "Shift") {
            mkey.m_modifier.press(Modifier::Type_Shift);
        }
        else if (mod == "Ctrl" || mod == "Control") {
            mkey.m_modifier.press(Modifier::Type_Control);
        }
        else if (mod == "Alt") {
            mkey.m_modifier.press(Modifier::Type_Alt);
        }
        else if (mod == "Win" || mod == "Windows") {
            mkey.m_modifier.press(Modifier::Type_Windows);
        }
        // Check if it's a virtual modifier (M00-MFF)
        else if (mod.length() == 3 && mod[0] == 'M') {
            // Try to parse as virtual modifier
            try {
                uint8_t modNum = static_cast<uint8_t>(std::stoi(mod.substr(1), nullptr, 16));
                mkey.setVirtualMod(modNum, true);
            } catch (const std::exception& e) {
                logError("Invalid virtual modifier '" + mod + "' in expression '" + from_spec + "': " + e.what());
                return ModifiedKey();
            }
        }
        else {
            logError("Unknown modifier '" + mod + "' in expression '" + from_spec + "'");
            return ModifiedKey();
        }
    }

    return mkey;
}

bool JsonConfigLoader::validateSchema(const nlohmann::json& config)
{
    // Validate root is an object
    if (!config.is_object()) {
        logError("Configuration root must be a JSON object");
        return false;
    }

    // Check "version" field exists
    if (!config.contains("version")) {
        logError("Missing required 'version' field");
        return false;
    }

    // Validate version is a string
    if (!config["version"].is_string()) {
        logError("'version' field must be a string");
        return false;
    }

    // Check version is "2.0"
    std::string version = config["version"].get<std::string>();
    if (version != "2.0") {
        logError("Unsupported version '" + version + "': expected '2.0'");
        return false;
    }

    // Check required sections present
    if (!config.contains("keyboard")) {
        logError("Missing required 'keyboard' section");
        return false;
    }

    // Optional sections don't need validation here
    // (virtualModifiers and mappings are checked in their respective parsers)

    return true;
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
