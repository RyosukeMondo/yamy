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

    // TODO: Implement in task 1.4
    // - Parse "keyboard.keys" section
    // - Parse scan codes from hex strings (e.g., "0x1e" → 0x1e)
    // - Create Key objects and populate Keyboard::m_table
    // - Build m_keyLookup map for name resolution

    return false;
}

bool JsonConfigLoader::parseVirtualModifiers(const nlohmann::json& obj, Setting* setting)
{
    Expects(setting != nullptr);

    // TODO: Implement in task 1.6
    // - Parse "virtualModifiers" section
    // - Extract M00-MFF numbers from modifier names
    // - Register trigger keys in Setting::m_virtualModTriggers
    // - Register tap actions in Setting::m_modTapActions

    return false;
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
    // TODO: Implement in task 1.5
    // - Lookup key by name in m_keyLookup map
    // - Return nullptr for unknown keys with helpful error message

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

    // TODO: Implement in task 1.4
    // - Parse hex string (e.g., "0x1e", "0x3a")
    // - Validate format (must start with "0x")
    // - Convert to uint16_t

    return false;
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
