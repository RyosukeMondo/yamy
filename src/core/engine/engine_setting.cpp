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

#include <iomanip>
#include <string>
#include <filesystem>
#include <thread>
#include <chrono>


// manageTs4mayu removed (moved to InputDriver)



// set m_setting
bool Engine::setSetting(Setting *i_setting) {
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
    // Clear existing table
    m_substitutionTable.clear();

    // Iterate through all substitutions defined in .mayu file
    // Substitutes are stored as std::list<Substitute> in m_substitutes
    for (const auto& substitute : keyboard.getSubstitutes()) {
        // Each Substitute contains:
        // - m_mkeyFrom: Source key (with modifiers)
        // - m_mkeyTo: Target key (with modifiers)

        // Extract YAMY scan codes from Key objects
        // Note: For now, we ignore modifiers and only map the raw scan codes
        // This matches the current EventProcessor design which operates on scan codes

        const Key* fromKey = substitute.m_mkeyFrom.m_key;
        const Key* toKey = substitute.m_mkeyTo.m_key;

        if (!fromKey || !toKey) {
            // Skip invalid substitutions
            continue;
        }

        // Get scan codes (each key may have multiple scan codes)
        const ScanCode* fromScans = fromKey->getScanCodes();
        const ScanCode* toScans = toKey->getScanCodes();
        size_t fromSize = fromKey->getScanCodesSize();
        size_t toSize = toKey->getScanCodesSize();

        if (fromSize == 0 || toSize == 0) {
            // Skip keys without scan codes
            continue;
        }

        // Use the first scan code from each key
        // The m_scan field is the actual YAMY scan code (uint16_t)
        uint16_t fromYamyScan = fromScans[0].m_scan;
        uint16_t toYamyScan = toScans[0].m_scan;

        // Add to substitution table: fromYamyScan → toYamyScan
        m_substitutionTable[fromYamyScan] = toYamyScan;

        // Log the substitution for debugging
        {
            Acquire a(&m_log, 1);
            m_log << "Substitution: 0x" << std::hex << std::setfill('0')
                  << std::setw(4) << fromYamyScan
                  << " → 0x" << std::setw(4) << toYamyScan
                  << std::dec << std::endl;
        }
    }

    // Log summary
    {
        Acquire a(&m_log, 0);
        m_log << "Built substitution table with " << m_substitutionTable.size()
              << " mappings" << std::endl;
    }

    // Create or recreate EventProcessor with new substitution table
    m_eventProcessor = std::make_unique<yamy::EventProcessor>(m_substitutionTable);

    // Register number modifiers from .mayu configuration
    for (const auto& numberMod : keyboard.getNumberModifiers()) {
        const Key* numberKey = numberMod.m_numberKey;
        const Key* modifierKey = numberMod.m_modifierKey;

        if (!numberKey || !modifierKey) {
            continue;
        }

        // Get YAMY scan codes
        const ScanCode* numberScans = numberKey->getScanCodes();
        const ScanCode* modifierScans = modifierKey->getScanCodes();
        size_t numberSize = numberKey->getScanCodesSize();
        size_t modifierSize = modifierKey->getScanCodesSize();

        if (numberSize == 0 || modifierSize == 0) {
            continue;
        }

        uint16_t numberYamyScan = numberScans[0].m_scan;
        uint16_t modifierYamyScan = modifierScans[0].m_scan;

        m_eventProcessor->registerNumberModifier(numberYamyScan, modifierYamyScan);

        // Log the number modifier registration
        {
            Acquire a(&m_log, 1);
            m_log << "Number Modifier: 0x" << std::hex << std::setfill('0')
                  << std::setw(4) << numberYamyScan
                  << " → 0x" << std::setw(4) << modifierYamyScan
                  << std::dec << std::endl;
        }
    }

    {
        Acquire a(&m_log, 0);
        m_log << "Registered " << keyboard.getNumberModifiers().size()
              << " number modifiers" << std::endl;
    }

    // Register modal modifiers from .mayu configuration
    // Modal modifiers (mod mod9 = !!A) are stored in keymap modifier assignments
    // Iterate through all keymaps to find modal modifier assignments
    int modalModCount = 0;
    const auto& keymapList = m_setting->m_keymaps.getKeymapList();
    for (const auto& keymap : keymapList) {
        // Check for modal modifiers (Type_Mod0 through Type_Mod19)
        for (int modType = Modifier::Type_Mod0; modType <= Modifier::Type_Mod19; ++modType) {
            const auto& modAssignments = keymap.getModAssignments(static_cast<Modifier::Type>(modType));

            for (const auto& assignment : modAssignments) {
                if (assignment.m_key && assignment.m_assignMode == Keymap::AM_oneShot) {
                    // Found a modal modifier assignment (!! operator)
                    // Extract trigger key scan code
                    const ScanCode* scans = assignment.m_key->getScanCodes();
                    size_t scanSize = assignment.m_key->getScanCodesSize();

                    if (scanSize > 0) {
                        uint16_t triggerYamyScan = scans[0].m_scan;

                        // Register modal modifier with EventProcessor's ModifierKeyHandler
                        if (m_eventProcessor && m_eventProcessor->getModifierHandler()) {
                            m_eventProcessor->getModifierHandler()->registerModalModifier(
                                triggerYamyScan,
                                modType  // Modifier::Type enum value
                            );
                        }

                        {
                            Acquire a(&m_log, 1);
                            m_log << "Modal Modifier: mod" << (modType - Modifier::Type_Mod0)
                                  << " = !!" << assignment.m_key->getName()
                                  << " (0x" << std::hex << std::setfill('0')
                                  << std::setw(4) << triggerYamyScan
                                  << std::dec << ") - REGISTERED" << std::endl;
                        }

                        modalModCount++;
                    }
                }
            }
        }
    }

    {
        Acquire a(&m_log, 0);
        m_log << "Registered " << modalModCount
              << " modal modifiers"
              << std::endl;
    }

    // Enable debug logging if YAMY_DEBUG_KEYCODE env var is set
    const char* debugEnv = std::getenv("YAMY_DEBUG_KEYCODE");
    if (debugEnv && std::string(debugEnv) == "1") {
        // Set platform logger to DEBUG level
        yamy::platform::PlatformLogger::instance().setLevel(yamy::platform::LogLevel::DEBUG);

        m_eventProcessor->setDebugLogging(true);
        Acquire a(&m_log, 0);
        m_log << "EventProcessor debug logging enabled" << std::endl;
    }
}
