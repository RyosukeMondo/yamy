#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// setting.h


#ifndef _SETTING_H
#  define _SETTING_H


#  include "keymap.h"
#  include "parser.h"
#  include "multithread.h"
#  include "../utils/config_store.h"
#  include <set>


/// this class contains all of loaded settings
class Setting
{
public:
    typedef std::set<tstringi> Symbols;        ///
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
    LONG m_dragThreshold;            ///
    unsigned int m_oneShotRepeatableDelay;    ///

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
typedef std::list<tstringi> HomeDirectories;
extern void getHomeDirectories(const ConfigStore *i_config, HomeDirectories *o_path);

/// get mayu filename from config
extern bool getFilenameFromConfig(const ConfigStore &i_config,
    tstringi *o_name, tstringi *o_filename, Setting::Symbols *o_symbols);


#endif // !_SETTING_H
