#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// setting_loader.h


#ifndef _SETTING_LOADER_H
#  define _SETTING_LOADER_H

#  include "setting.h"
#  include "parser.h"
#  include "../utils/config_store.h"
#  include "include_context.h"
#  include <iostream>

///
///
template <typename Derived, typename... Args>
class Command;

class SettingLoader
{
    template <typename Derived, typename... Args>
    friend class Command;
#  define FUNCTION_FRIEND
#  include "../functions/function_friends.h"
#  undef FUNCTION_FRIEND

public:
    ///
    class FunctionCreator
    {
    public:
        const char *m_name;            ///
        FunctionData *m_creator;            ///
    };

private:
    typedef std::vector<Token> Tokens;        ///
    typedef std::vector<std::string> Prefixes;    ///
    typedef std::vector<bool> CanReadStack;    ///

private:
    Setting *m_setting;                /// loaded setting
    const ConfigStore *m_config;        /// config store
    bool m_isThereAnyError;            /// is there any error ?

    SyncObject *m_soLog;                /// guard log output stream
    std::ostream *m_log;                /// log output stream

    yamy::IncludeContext* m_includeContext; /// include tracking (owned if root loader)
    bool m_ownsIncludeContext;          /// true if this loader owns the context

    std::string m_currentFilename;            /// current filename

    Tokens m_tokens;                /// tokens for current line
    Tokens::iterator m_ti;            /// current processing token

    static Prefixes *m_prefixes;            /// prefix terminal symbol
    static size_t m_prefixesRefCcount;        /// reference count of prefix

    Keymap *m_currentKeymap;            /// current keymap

    CanReadStack m_canReadStack;            /// for &lt;COND_SYMBOL&gt;

    Modifier m_defaultAssignModifier;        /** default
                                                    &lt;ASSIGN_MODIFIER&gt; */
    Modifier m_defaultKeySeqModifier;        /** default
                                                    &lt;KEYSEQ_MODIFIER&gt; */

private:
    bool isEOL();                    /// is there no more tokens ?
    Token *getToken();                /// get next token
    Token *lookToken();                /// look next token
    bool getOpenParen(bool i_doesThrow, const char *i_name); /// argument "("
    bool getCloseParen(bool i_doesThrow, const char *i_name); /// argument ")"
    bool getComma(bool i_doesThrow, const char *i_name); /// argument ","

    void load_LINE();                /// &lt;LINE&gt;
    void load_DEFINE();                /// &lt;DEFINE&gt;
    void load_IF();                /// &lt;IF&gt;
    void load_ELSE(bool i_isElseIf, const std::string &i_token);
    /// &lt;ELSE&gt; &lt;ELSEIF&gt;
    bool load_ENDIF(const std::string &i_token);    /// &lt;ENDIF&gt;
    void load_INCLUDE();                /// &lt;INCLUDE&gt;
    void load_SCAN_CODES(Key *o_key);        /// &lt;SCAN_CODES&gt;
    void load_DEFINE_KEY();            /// &lt;DEFINE_KEY&gt;
    void load_DEFINE_MODIFIER();            /// &lt;DEFINE_MODIFIER&gt;
    void load_DEFINE_SYNC_KEY();            /// &lt;DEFINE_SYNC_KEY&gt;
    void load_DEFINE_ALIAS();            /// &lt;DEFINE_ALIAS&gt;
    void load_DEFINE_SUBSTITUTE();        /// &lt;DEFINE_SUBSTITUTE&gt;
    void load_DEFINE_NUMBER_MODIFIER();    /// &lt;DEFINE_NUMBER_MODIFIER&gt;
    void load_DEFINE_OPTION();            /// &lt;DEFINE_OPTION&gt;
    void load_KEYBOARD_DEFINITION();        /// &lt;KEYBOARD_DEFINITION&gt;
    Modifier load_MODIFIER(Modifier::Type i_mode, Modifier i_modifier,
                           Modifier::Type *o_mode = nullptr);
    /// &lt;..._MODIFIER&gt;
    Key *load_KEY_NAME();                /// &lt;KEY_NAME&gt;
    Key *createVirtualKey(const std::string &i_name, uint16_t i_keycode);  /// Create virtual key
    void load_KEYMAP_DEFINITION(const Token *i_which);
    /// &lt;KEYMAP_DEFINITION&gt;
    void load_ARGUMENT(bool *o_arg);        /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(int *o_arg);        /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(unsigned int *o_arg);    /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(long *o_arg);        /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(unsigned long *o_arg);    /// &lt;ARGUMENT&gt;
#ifdef _WIN32
    // On Windows, __int64 is distinct from long
    void load_ARGUMENT(unsigned __int64 *o_arg);    /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(__int64 *o_arg);        /// &lt;ARGUMENT&gt;
#endif
    void load_ARGUMENT(stringq *o_arg);        /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(std::string *o_arg);    /// &lt;ARGUMENT&gt;
#ifdef _WIN32
    // On Windows with _UNICODE, tstring is wstring
    void load_ARGUMENT(tstring *o_arg);        /// &lt;ARGUMENT&gt;
#endif
    void load_ARGUMENT(std::list<stringq> *o_arg); /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(std::list<std::string> *o_arg); /// &lt;ARGUMENT&gt;
#ifdef _WIN32
    // On Windows with _UNICODE, list<tstring> is list<wstring>
    void load_ARGUMENT(std::list<tstring> *o_arg); /// &lt;ARGUMENT&gt;
#endif
    void load_ARGUMENT(Regex *o_arg);        /// &lt;ARGUMENT&gt;
    // Note: tregex is an alias for Regex, so no separate overload needed
    void load_ARGUMENT(VKey *o_arg);        /// &lt;ARGUMENT_VK&gt;
    void load_ARGUMENT(ToWindowType *o_arg);    /// &lt;ARGUMENT_WINDOW&gt;
    void load_ARGUMENT(GravityType *o_arg);    /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(MouseHookType *o_arg);    /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(MayuDialogType *o_arg);    /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(ModifierLockType *o_arg);    /// &lt;ARGUMENT_LOCK&gt;
    void load_ARGUMENT(ToggleType *o_arg);    /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(ShowCommandType *o_arg);    ///&lt;ARGUMENT_SHOW_WINDOW&gt;
    void load_ARGUMENT(TargetWindowType *o_arg);
    /// &lt;ARGUMENT_TARGET_WINDOW_TYPE&gt;
    void load_ARGUMENT(BooleanType *o_arg);    /// &lt;bool&gt;
    void load_ARGUMENT(LogicalOperatorType *o_arg);/// &lt;ARGUMENT&gt;
    void load_ARGUMENT(Modifier *o_arg);        /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(const Keymap **o_arg);    /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(const KeySeq **o_arg);    /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(StrExprArg *o_arg);    /// &lt;ARGUMENT&gt;
    void load_ARGUMENT(WindowMonitorFromType *o_arg);    /// &lt;ARGUMENT&gt;
    KeySeq *load_KEY_SEQUENCE(
        const std::string &i_name = "", bool i_isInParen = false,
        Modifier::Type i_mode = Modifier::Type_KEYSEQ); /// &lt;KEY_SEQUENCE&gt;
    void load_KEY_ASSIGN();            /// &lt;KEY_ASSIGN&gt;
    void load_EVENT_ASSIGN();            /// &lt;EVENT_ASSIGN&gt;
    void load_MODIFIER_ASSIGNMENT();        /// &lt;MODIFIER_ASSIGN&gt;
    void load_MOD_ASSIGN();            /// &lt;MOD_ASSIGN&gt; (for M00-MFF tap actions)
    void load_LOCK_ASSIGNMENT();            /// &lt;LOCK_ASSIGN&gt;
    void load_KEYSEQ_DEFINITION();        /// &lt;KEYSEQ_DEFINITION&gt;

    // Helper functions for load_MODIFIER
    bool parseMxxModifier(const std::string& token_str, Modifier::Type i_mode, Modifier& i_modifier, Modifier::Type* o_mode, Modifier& isModifierSpecified, int& flag);
    bool parseLxxModifier(const std::string& token_str, Modifier::Type i_mode, Modifier& i_modifier, Modifier::Type* o_mode, Modifier& isModifierSpecified, int& flag);

    /// load
    void load(const std::string &i_filename);

    /// is the filename readable ?
    bool isReadable(const std::string &i_filename, int i_debugLevel = 1) const;

    /// get filename
    bool getFilename(const std::string &i_name,
                     std::string *o_path, int i_debugLevel = 1) const;

public:
    /// Constructor for root loader (creates own IncludeContext)
    SettingLoader(SyncObject *i_soLog, std::ostream *i_log, const ConfigStore *i_config = nullptr);

    /// Constructor for child loader (shares IncludeContext)
    SettingLoader(SyncObject *i_soLog, std::ostream *i_log, const ConfigStore *i_config, yamy::IncludeContext& i_includeContext);

    /// Destructor
    ~SettingLoader();

    /// load setting
    bool load(Setting *o_setting, const std::string &i_filename = "");

    /// initialize setting
    bool initialize(Setting *o_setting);

    /// load setting from data string
    void loadFromData(const std::string &data);
};

#endif // !_SETTING_LOADER_H
