#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// setting.h


#ifndef _SETTING_H
#  define _SETTING_H


#  include "../input/keymap.h"
#  include "parser.h"
#  include "multithread.h"
#  include "../utils/config_store.h"
#  include <set>
#  include <unordered_map>


/// this class contains all of loaded settings
class Setting
{
public:
    typedef std::set<std::string> Symbols;        ///
    typedef std::list<Modifier> Modifiers;    ///

public:
    Keyboard m_keyboard;                ///
    Keymaps m_keymaps;                ///
    KeySeqs m_keySeqs;                ///
    Symbols m_symbols;                ///
    bool m_correctKanaLockHandling;        ///
    bool m_sts4mayu;                ///
    bool m_cts4mayu;                ///
    bool m_mouseEvent;                ///
    int32_t m_dragThreshold;            ///
    unsigned int m_oneShotRepeatableDelay;    ///
    std::unordered_map<uint8_t, uint16_t> m_modTapActions;  /// Tap actions for M00-MFF modifiers
    std::unordered_map<uint16_t, uint8_t> m_virtualModTriggers; /// Trigger keys for M00-MFF modifiers

public:
    Setting()
            : m_correctKanaLockHandling(false),
            m_sts4mayu(false),
            m_cts4mayu(false),
            m_mouseEvent(false),
            m_dragThreshold(0),
            m_oneShotRepeatableDelay(0) { }
};


///
namespace Event
{
///
extern Key prefixed;
///
extern Key before_key_down;
///
extern Key after_key_up;
///
extern Key *events[];
}


///
class SettingLoader;


/// get home directory path
typedef std::list<std::string> HomeDirectories;
extern void getHomeDirectories(const ConfigStore *i_config, HomeDirectories *o_path);

/// get mayu filename from config
extern bool getFilenameFromConfig(const ConfigStore &i_config,
    std::string *o_name, std::string *o_filename, Setting::Symbols *o_symbols);


#endif // !_SETTING_H
