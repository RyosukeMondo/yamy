//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_setting.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "../platform/hook_interface.h"
#include "mayurc.h"
#include "setting_loader.h"
#include "stringtool.h"
#include "windowstool.h"
#include "modifier_key_handler.h"
#include "../../utils/platform_logger.h"
#include "compiled_rule.h"
#include "lookup_table.h"
#include "engine_event_processor.h"

#include <iomanip>
#include <string>
#include <filesystem>
#include <thread>
#include <chrono>
#include <gsl/gsl>

// manageTs4mayu removed (moved to InputDriver)



// set m_setting
bool Engine::setSetting(Setting *i_setting) {
    Expects(i_setting != nullptr);

    Acquire a(&m_cs);
    if (m_isSynchronizing)
        return false;

    if (m_setting) {
        for (Keyboard::KeyIterator i = m_setting->m_keyboard.getKeyIterator();
                *i; ++ i) {
            Key *key = i_setting->m_keyboard.searchKey(*(*i));
            if (key) {
                key->m_isPressed = (*i)->m_isPressed;
                key->m_isPressedOnWin32 = (*i)->m_isPressedOnWin32;
                key->m_isPressedByAssign = (*i)->m_isPressedByAssign;
            }
        }
        if (m_lastGeneratedKey)
            m_lastGeneratedKey =
                i_setting->m_keyboard.searchKey(*m_lastGeneratedKey);
        for (size_t i = 0; i < NUMBER_OF(m_lastPressedKey); ++ i)
            if (m_lastPressedKey[i])
                m_lastPressedKey[i] =
                    i_setting->m_keyboard.searchKey(*m_lastPressedKey[i]);
    }

    m_setting = i_setting;

    m_inputDriver->manageExtension("sts4mayu.dll", "SynCOM.dll",
                  m_setting->m_sts4mayu, (void**)&m_sts4mayu);
    m_inputDriver->manageExtension("cts4mayu.dll", "TouchPad.dll",
                  m_setting->m_cts4mayu, (void**)&m_cts4mayu);

    auto* hookData = yamy::platform::getHookData();
    hookData->m_correctKanaLockHandling = m_setting->m_correctKanaLockHandling;

    // Get global keymap (simplified - no per-window focus tracking)
    m_globalKeymap = m_setting->m_keymaps.searchByName("Global");
    if (!m_globalKeymap) {
        Acquire a(&m_log, 0);
        m_log << "Warning: No 'Global' keymap found, using first keymap" << std::endl;
        // Fallback: use first keymap if Global not found
        const auto& keymapList = m_setting->m_keymaps.getKeymapList();
        if (!keymapList.empty()) {
            m_globalKeymap = const_cast<Keymap*>(&keymapList.front());
        }
    }

    if (m_globalKeymap) {
        setCurrentKeymap(m_globalKeymap);
        Acquire a(&m_log, 0);
        m_log << "Loaded global keymap: " << (m_globalKeymap->getName().empty() ? "(unnamed)" : m_globalKeymap->getName()) << std::endl;
    } else {
        Acquire a(&m_log, 0);
        m_log << "Error: No keymaps available" << std::endl;
    }

    // Build substitution table and initialize EventProcessor
    buildSubstitutionTable(m_setting->m_keyboard);

    return true;
}


// Switch to a different configuration file
// Properly handles string conversions via to_tstring() for cross-platform compatibility
bool Engine::switchConfiguration(const std::string& configPath) {
    // GUARD: Prevent reloading the same config (fixes reload loop bug)
    if (m_currentConfigPath == configPath) {
        return true;  // Already loaded, consider this success
    }

#ifdef _WIN32
    // Windows stub - not yet implemented due to wide stream incompatibility
    // SettingLoader expects std::ostream* but m_log is std::wostream-based on Windows
    Acquire a(&m_log, 0);
    m_log << "switchConfiguration: not supported on Windows yet" << std::endl;
    notifyGUI(yamy::MessageType::ConfigError, "Configuration switching not supported on Windows");
    if (m_configSwitchCallback) {
        m_configSwitchCallback(false, configPath);
    }
    return false;
#else
    // Linux implementation
    namespace fs = std::filesystem;

    notifyGUI(yamy::MessageType::ConfigLoading, configPath);

    // Validate config path exists
    if (!fs::exists(configPath) || !fs::is_regular_file(configPath)) {
        Acquire a(&m_log, 0);
        m_log << "switchConfiguration: file not found: " << to_tstring(configPath) << std::endl;
        if (m_configSwitchCallback) {
            m_configSwitchCallback(false, configPath);
        }
        notifyGUI(yamy::MessageType::ConfigError, "File not found");
        return false;
    }

    // Create new setting
    Setting* newSetting = new Setting;

    // Try to parse the new config file
    bool parseSuccess = false;
    try {
        SettingLoader loader(&m_log, &m_log, m_configStore);
        parseSuccess = loader.load(newSetting, configPath);
    } catch (const std::exception& e) {
        Acquire a(&m_log, 0);
        m_log << "switchConfiguration: parse exception: " << e.what() << std::endl;
        parseSuccess = false;
        notifyGUI(yamy::MessageType::ConfigError, e.what());
    } catch (...) {
        Acquire a(&m_log, 0);
        m_log << "switchConfiguration: unknown parse exception" << std::endl;
        parseSuccess = false;
        notifyGUI(yamy::MessageType::ConfigError, "Unknown parsing error");
    }

    if (!parseSuccess) {
        // Rollback: delete the failed setting and keep current
        delete newSetting;
        Acquire a(&m_log, 0);
        m_log << "switchConfiguration: failed to parse config: " << to_tstring(configPath) << std::endl;
        if (m_configSwitchCallback) {
            m_configSwitchCallback(false, configPath);
        }
        // Assuming the loader logs the specific error, a generic message is sent.
        notifyGUI(yamy::MessageType::ConfigError, "Failed to parse config");
        return false;
    }

    // Store old setting for cleanup
    Setting* oldSetting = m_setting;

    // Try to apply the new setting
    // setSetting may fail if synchronizing, retry a few times
    int retryCount = 0;
    const int maxRetries = 10;
    while (!setSetting(newSetting) && retryCount < maxRetries) {
        // Brief sleep to allow synchronization to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        retryCount++;
    }

    if (retryCount >= maxRetries) {
        // Failed to set new setting, rollback
        delete newSetting;
        Acquire a(&m_log, 0);
        m_log << "switchConfiguration: failed to apply setting (engine busy): "
              << to_tstring(configPath) << std::endl;
        if (m_configSwitchCallback) {
            m_configSwitchCallback(false, configPath);
        }
        notifyGUI(yamy::MessageType::ConfigError, "Failed to apply setting (engine busy)");
        return false;
    }

    // Clean up old setting
    delete oldSetting;

    // Store the current config path to prevent reload loops
    // IMPORTANT: Do this BEFORE ConfigManager update to prevent callback loops
    m_currentConfigPath = configPath;

    // Update ConfigManager with the new active config
    ConfigManager::instance().setActiveConfig(configPath);

    // Log success
    {
        Acquire a(&m_log, 0);
        m_log << "switchConfiguration: successfully switched to: " << to_tstring(configPath) << std::endl;
    }

    // Notify callback
    if (m_configSwitchCallback) {
        m_configSwitchCallback(true, configPath);
    }

    notifyGUI(yamy::MessageType::ConfigLoaded, configPath);
    return true;
#endif // _WIN32
}


// Build substitution table from Keyboard::Substitutes
void Engine::buildSubstitutionTable(const Keyboard &keyboard) {
    // GUARD: Prevent rebuilding during event processing
    if (m_generateKeyboardEventsRecursionGuard > 0) {
        return;
    }

    // Create or recreate EventProcessor
    m_eventProcessor = std::make_unique<yamy::EventProcessor>();
    
    // Get the new lookup table and compile rules into it
    int total_rules = 0;
    if (auto* lookupTable = m_eventProcessor->getLookupTable()) {
        lookupTable->clear();
        for (const auto& substitute : keyboard.getSubstitutes()) {
            const Key* fromKey = substitute.m_mkeyFrom.m_key;
            if (!fromKey || fromKey->getScanCodesSize() == 0) {
                continue;
            }
            uint16_t inputScanCode = fromKey->getScanCodes()[0].m_scan;
            std::vector<yamy::engine::CompiledRule> rules = this->compileSubstitute(substitute);
            for (const auto& rule : rules) {
                lookupTable->addRule(inputScanCode, rule);
            }
            total_rules += rules.size();
        }
    }

    // Log summary
    {
        Acquire a(&m_log, 0);
        m_log << "Built new rule lookup table with " << total_rules
              << " compiled rules from " << keyboard.getSubstitutes().size()
              << " substitutes." << std::endl;
    }

    m_eventProcessor->setDebugLogging(true);

    // Register number modifiers
    for (const auto& numberMod : keyboard.getNumberModifiers()) {
        if (!numberMod.m_numberKey || !numberMod.m_modifierKey || numberMod.m_numberKey->getScanCodesSize() == 0 || numberMod.m_modifierKey->getScanCodesSize() == 0) {
            continue;
        }
        uint16_t numberYamyScan = numberMod.m_numberKey->getScanCodes()[0].m_scan;
        uint16_t modifierYamyScan = numberMod.m_modifierKey->getScanCodes()[0].m_scan;
        m_eventProcessor->registerNumberModifier(numberYamyScan, modifierYamyScan);
    }

    // Register virtual modifiers triggers
    if (m_setting) {
        for (const auto& [trigger, modNum] : m_setting->m_virtualModTriggers) {
            uint16_t tapOutput = 0;
            auto it = m_setting->m_modTapActions.find(modNum);
            if (it != m_setting->m_modTapActions.end()) {
                tapOutput = it->second;
            }
            // Register: trigger -> modNum (with optional tapOutput)
            m_eventProcessor->registerVirtualModifierTrigger(trigger, modNum, tapOutput);
        }
    }

    // Enable debug logging if env var is set
    const char* debugEnv = std::getenv("YAMY_DEBUG_KEYCODE");
    if (debugEnv && std::string(debugEnv) == "1") {
        m_eventProcessor->setDebugLogging(true);
    }
}
