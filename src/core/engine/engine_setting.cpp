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

namespace {

std::vector<yamy::engine::CompiledRule> compileSubstitute(const Keyboard::Substitute& sub) {
    using namespace yamy::input;

    std::vector<yamy::engine::CompiledRule> rules;
    yamy::engine::CompiledRule base_rule;

    // --- Compile Output ---
    const Key* toKey = sub.m_mkeyTo.m_key;
    if (toKey && toKey->getScanCodesSize() > 0) {
        base_rule.outputScanCode = toKey->getScanCodes()[0].m_scan;
    } else {
        base_rule.outputScanCode = 0;
    }

    // --- Compile Input Conditions ---
    const Modifier& fromMod = sub.m_mkeyFrom.m_modifier;
    std::vector<std::pair<size_t, size_t>> generic_modifiers;

    // --- Handle Generic Modifiers (Shift, Ctrl, Alt, Win) ---
    if (fromMod.isOn(Modifier::Type_Shift)) {
        generic_modifiers.push_back({ModifierState::LSHIFT, ModifierState::RSHIFT});
    } else if (!fromMod.isDontcare(Modifier::Type_Shift)) {
        base_rule.requiredOff.set(ModifierState::LSHIFT);
        base_rule.requiredOff.set(ModifierState::RSHIFT);
    }

    if (fromMod.isOn(Modifier::Type_Control)) {
        generic_modifiers.push_back({ModifierState::LCTRL, ModifierState::RCTRL});
    } else if (!fromMod.isDontcare(Modifier::Type_Control)) {
        base_rule.requiredOff.set(ModifierState::LCTRL);
        base_rule.requiredOff.set(ModifierState::RCTRL);
    }

    if (fromMod.isOn(Modifier::Type_Alt)) {
        generic_modifiers.push_back({ModifierState::LALT, ModifierState::RALT});
    } else if (!fromMod.isDontcare(Modifier::Type_Alt)) {
        base_rule.requiredOff.set(ModifierState::LALT);
        base_rule.requiredOff.set(ModifierState::RALT);
    }

    if (fromMod.isOn(Modifier::Type_Windows)) {
        generic_modifiers.push_back({ModifierState::LWIN, ModifierState::RWIN});
    } else if (!fromMod.isDontcare(Modifier::Type_Windows)) {
        base_rule.requiredOff.set(ModifierState::LWIN);
        base_rule.requiredOff.set(ModifierState::RWIN);
    }

    // --- Handle Specific State Modifiers ---
    // Note: ModifierState combines physical locks (CapsLock) and virtual states (Up) in StdModifier
    if (fromMod.isOn(Modifier::Type_CapsLock)) {
        base_rule.requiredOn.set(ModifierState::CAPSLOCK);
    } else if (!fromMod.isDontcare(Modifier::Type_CapsLock)) {
        base_rule.requiredOff.set(ModifierState::CAPSLOCK);
    }
    if (fromMod.isOn(Modifier::Type_NumLock)) {
        base_rule.requiredOn.set(ModifierState::NUMLOCK);
    } else if (!fromMod.isDontcare(Modifier::Type_NumLock)) {
        base_rule.requiredOff.set(ModifierState::NUMLOCK);
    }
    if (fromMod.isOn(Modifier::Type_ScrollLock)) {
        base_rule.requiredOn.set(ModifierState::SCROLLLOCK);
    } else if (!fromMod.isDontcare(Modifier::Type_ScrollLock)) {
        base_rule.requiredOff.set(ModifierState::SCROLLLOCK);
    }
     if (fromMod.isOn(Modifier::Type_Up)) {
        base_rule.requiredOn.set(ModifierState::UP);
    } else if (!fromMod.isDontcare(Modifier::Type_Up)) {
        base_rule.requiredOff.set(ModifierState::UP);
    }
    // TODO: Add other specific modifiers like KanaLock if they get added to ModifierState

    // --- Handle Virtual Modifiers (M00-MFF) ---
    for (int i = 0; i < 256; ++i) {
        if (sub.m_mkeyFrom.isVirtualModActive(i)) {
            base_rule.requiredOn.set(ModifierState::VIRTUAL_OFFSET + i);
        }
        // NOTE: isVirtualModActive does not have a "required off" state in legacy model.
        // It's assumed to be off if not specified as on.
        else {
             base_rule.requiredOff.set(ModifierState::VIRTUAL_OFFSET + i);
        }
    }
    
    // --- Handle Lock Modifiers (L00-LFF) ---
    for (int i = 0; i < 10; ++i) {
        Modifier::Type lockType = static_cast<Modifier::Type>(Modifier::Type_Lock0 + i);
        if (fromMod.isOn(lockType)) {
            base_rule.requiredOn.set(ModifierState::LOCK_OFFSET + i);
        } else if (!fromMod.isDontcare(lockType)) {
            base_rule.requiredOff.set(ModifierState::LOCK_OFFSET + i);
        }
    }


    // --- Expand Generic Modifiers ---
    // If there are N generic 'on' modifiers, we need 2^N rules.
    if (generic_modifiers.empty()) {
        rules.push_back(base_rule);
    } else {
        size_t num_expansions = 1 << generic_modifiers.size();
        for (size_t i = 0; i < num_expansions; ++i) {
            yamy::engine::CompiledRule new_rule = base_rule;
            for (size_t j = 0; j < generic_modifiers.size(); ++j) {
                // Use j-th bit of i to decide between left/right version
                if ((i >> j) & 1) {
                    new_rule.requiredOn.set(generic_modifiers[j].second); // e.g., RSHIFT
                } else {
                    new_rule.requiredOn.set(generic_modifiers[j].first);  // e.g., LSHIFT
                }
            }
            rules.push_back(new_rule);
        }
    }

    return rules;
}

} // namespace


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
    if (m_currentFocusOfThread) {
        for (FocusOfThreads::iterator i = m_focusOfThreads.begin();
                i != m_focusOfThreads.end(); i ++) {
            FocusOfThread *fot = &(*i).second;
            m_setting->m_keymaps.searchWindow(&fot->m_keymaps,
                                              fot->m_className, fot->m_titleName);
        }
    }
    m_setting->m_keymaps.searchWindow(&m_globalFocus.m_keymaps, "", "");
    if (m_globalFocus.m_keymaps.empty()) {
        Acquire a(&m_log, 0);
        m_log << "internal error: m_globalFocus.m_keymap is empty"
        << std::endl;
    }
    m_currentFocusOfThread = &m_globalFocus;
    setCurrentKeymap(m_globalFocus.m_keymaps.front());
    m_hwndFocus = nullptr;

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

    // Create a temporary, simple substitution table for layer 1.
    // This is still needed for things like virtual modifier triggers.
    m_substitutionTable.clear();
     for (const auto& substitute : keyboard.getSubstitutes()) {
        const Key* fromKey = substitute.m_mkeyFrom.m_key;
        const Key* toKey = substitute.m_mkeyTo.m_key;
        if (!fromKey || !toKey || fromKey->getScanCodesSize() == 0 || toKey->getScanCodesSize() == 0) {
            continue;
        }
        uint16_t fromYamyScan = fromKey->getScanCodes()[0].m_scan;
        uint16_t toYamyScan = toKey->getScanCodes()[0].m_scan;
        m_substitutionTable[fromYamyScan] = toYamyScan;
    }

    // Create or recreate EventProcessor
    m_eventProcessor = std::make_unique<yamy::EventProcessor>(m_substitutionTable);
    
    // Get the new lookup table and compile rules into it
    if (auto* lookupTable = m_eventProcessor->getLookupTable()) {
        lookupTable->clear();
        int total_rules = 0;
        for (const auto& substitute : keyboard.getSubstitutes()) {
            const Key* fromKey = substitute.m_mkeyFrom.m_key;
            if (!fromKey || fromKey->getScanCodesSize() == 0) {
                continue;
            }
            uint16_t inputScanCode = fromKey->getScanCodes()[0].m_scan;
            std::vector<yamy::engine::CompiledRule> rules = compileSubstitute(substitute);
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
    if (m_setting && !m_setting->m_modTapActions.empty()) {
        for (const auto& [mod_num, tap_action_yamy] : m_setting->m_modTapActions) {
            uint16_t virtual_mod_code = 0xF000 + mod_num;
            for (const auto& [from_yamy, to_yamy] : m_substitutionTable) {
                if (to_yamy == virtual_mod_code) {
                    m_eventProcessor->registerVirtualModifierTrigger(from_yamy, mod_num, tap_action_yamy);
                }
            }
        }
    }

    // Enable debug logging if env var is set
    const char* debugEnv = std::getenv("YAMY_DEBUG_KEYCODE");
    if (debugEnv && std::string(debugEnv) == "1") {
        m_eventProcessor->setDebugLogging(true);
    }
}
