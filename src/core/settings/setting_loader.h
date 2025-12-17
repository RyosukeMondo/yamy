#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// setting_loader.h - DEPRECATED stub for backward compatibility
//
// SettingLoader has been replaced by JsonConfigLoader for configuration loading.
// This stub exists only to allow compilation of legacy command code that
// references SettingLoader but doesn't actually use it.
//
// The command load() methods are no longer called since JSON configuration
// creates commands directly without text parsing.

#ifndef _SETTING_LOADER_H
#define _SETTING_LOADER_H

#include <ostream>

// Forward declarations
class Setting;

/**
 * @brief DEPRECATED - Use JsonConfigLoader instead
 *
 * This is a stub class to maintain compilation compatibility.
 * It is no longer used for actual configuration loading.
 */
class SettingLoader {
public:
    SettingLoader(std::ostream*, std::ostream*) {}

    // Stub methods referenced by command_base.h but never called
    bool getOpenParen(bool, const char*) { return false; }
    void getCloseParen(bool, const char*) {}
    bool getComma(bool, const char*) { return false; }

    // Template stub for load_ARGUMENT
    template<typename T>
    void load_ARGUMENT(T*) {}

    // Legacy load method - not used with JSON configs
    bool load(Setting*, const std::string&) { return false; }
};

#endif // _SETTING_LOADER_H
