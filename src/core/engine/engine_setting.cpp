//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_setting.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "../platform/hook_interface.h"
#include "mayurc.h"
#include "setting_loader.h"
#ifdef _WIN32
#include "windowstool.h"
#endif

#include <iomanip>
#ifdef _WIN32
#include <process.h>
#endif
#include <string>
#include <filesystem>
#ifndef _WIN32
#include <unistd.h>
#endif


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
    return true;
}


// Switch to a different configuration file
bool Engine::switchConfiguration(const std::string& configPath) {
    namespace fs = std::filesystem;

    notifyGUI(yamy::MessageType::ConfigLoading, configPath);

    // Validate config path exists
    if (!fs::exists(configPath) || !fs::is_regular_file(configPath)) {
        Acquire a(&m_log, 0);
        m_log << "switchConfiguration: file not found: " << configPath << std::endl;
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
        m_log << "switchConfiguration: failed to parse config: " << configPath << std::endl;
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
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100000);
#endif
        retryCount++;
    }

    if (retryCount >= maxRetries) {
        // Failed to set new setting, rollback
        delete newSetting;
        Acquire a(&m_log, 0);
        m_log << "switchConfiguration: failed to apply setting (engine busy): "
              << configPath << std::endl;
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
        m_log << "switchConfiguration: successfully switched to: " << configPath << std::endl;
    }

    // Notify callback
    if (m_configSwitchCallback) {
        m_configSwitchCallback(true, configPath);
    }

    notifyGUI(yamy::MessageType::ConfigLoaded, configPath);
    return true;
}
