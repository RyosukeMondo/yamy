//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// setting_loader.cpp


#include "misc.h"
#include "stringtool.h"
#include "utf_conversion.h"
#include "dlgsetting.h"
#include "errormessage.h"
#include "mayu.h"
#include "mayurc.h"
#include "setting.h"
#include "setting_loader.h"
#include <filesystem>
#include "windowstool.h"
#include "vkeytable.h"
#include "array.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <climits>
#include <sys/stat.h>
#include <cstdio>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SettingLoader

// is there no more tokens ?
bool SettingLoader::isEOL()
{
    return m_ti == m_tokens.end();
}


// get next token
Token *SettingLoader::getToken()
{
    if (isEOL())
        throw ErrorMessage() << "too few words.";
    return &*(m_ti ++);
}


// look next token
Token *SettingLoader::lookToken()
{
    if (isEOL())
        throw ErrorMessage() << "too few words.";
    return &*m_ti;
}


// argument "("
bool SettingLoader::getOpenParen(bool i_doesThrow, const char *i_name)
{
    if (!isEOL() && lookToken()->isOpenParen()) {
        getToken();
        return true;
    }
    if (i_doesThrow)
        throw ErrorMessage() << "there must be `(' after `&"
        << i_name << "'.";
    return false;
}


// argument ")"
bool SettingLoader::getCloseParen(bool i_doesThrow, const char *i_name)
{
    if (!isEOL() && lookToken()->isCloseParen()) {
        getToken();
        return true;
    }
    if (i_doesThrow)
        throw ErrorMessage() << "`&"  << i_name
        << "': too many arguments.";
    return false;
}


// argument ","
bool SettingLoader::getComma(bool i_doesThrow, const char *i_name)
{
    if (!isEOL() && lookToken()->isComma()) {
        getToken();
        return true;
    }
    if (i_doesThrow)
        throw ErrorMessage() << "`&"  << i_name
        << "': comma expected.";
    return false;
}


// <INCLUDE>
void SettingLoader::load_INCLUDE()
{
    std::string filename = to_UTF_8((*getToken()).getString());

    if (m_ast) {
        m_ast->includedFiles.push_back(filename);
    }

    try {
        // RAII guard for include stack - automatically handles push/pop
        yamy::IncludeGuard guard(*m_includeContext, filename);

        // Create child loader with SHARED include context and AST
        SettingLoader loader(m_soLog, m_log, m_config, *m_includeContext, m_ast);
        loader.m_currentFilename = m_currentFilename;
        loader.m_defaultAssignModifier = m_defaultAssignModifier;
        loader.m_defaultKeySeqModifier = m_defaultKeySeqModifier;

        if (!loader.load(m_setting, filename)) {
            m_isThereAnyError = true;
        }
    } catch (ErrorMessage &e) {
        // Circular include or depth exceeded
        if (m_log && m_soLog) {
            Acquire a(m_soLog);
            *m_log << m_currentFilename << " : error: " << e.getMessage() << std::endl;
        }
        m_isThereAnyError = true;
    }
}


// <SCAN_CODES>
void SettingLoader::load_SCAN_CODES(Key *o_key, std::vector<yamy::ast::ScanCodeDefinition>* o_ast_scan_codes)
{
    for (int j = 0; j < Key::MAX_SCAN_CODES_SIZE && !isEOL(); ++ j) {
        ScanCode sc;
        sc.m_flags = 0;
        
        yamy::ast::ScanCodeDefinition ast_sc;

        while (true) {
            Token *t = getToken();
            if (t->isNumber()) {
                sc.m_scan = (u_char)t->getNumber();
                if (o_key)
                    o_key->addScanCode(sc);
                
                if (o_ast_scan_codes) {
                    ast_sc.scan = sc.m_scan;
                    o_ast_scan_codes->push_back(ast_sc);
                }
                break;
            }
            
            std::string flag_str = t->getString();
            if (flag_str == "E0-") {
                sc.m_flags |= ScanCode::E0;
                if (o_ast_scan_codes) ast_sc.flags.push_back(flag_str);
            }
            else if (flag_str == "E1-") {
                sc.m_flags |= ScanCode::E1;
                if (o_ast_scan_codes) ast_sc.flags.push_back(flag_str);
            }
            else  throw ErrorMessage() << "`" << *t
                << "': invalid modifier.";
        }
    }
}


// <DEFINE_KEY>
void SettingLoader::load_DEFINE_KEY()
{
    yamy::ast::KeyDefinition key_def;
    Token *t = getToken();
    Key key;

    // <KEY_NAMES>
    if (*t == '(') {
        std::string name = getToken()->getString();
        key.addName(name);
        key_def.names.push_back(name);
        while (t = getToken(), *t != ')') {
            name = t->getString();
            key.addName(name);
            key_def.names.push_back(name);
        }
        if (*getToken() != "=")
            throw ErrorMessage() << "there must be `=' after `)'.";
    } else {
        std::string name = t->getString();
        key.addName(name);
        key_def.names.push_back(name);
        while (t = getToken(), *t != "=") {
            name = t->getString();
            key.addName(name);
            key_def.names.push_back(name);
        }
    }

    load_SCAN_CODES(&key, &key_def.scanCodes);
    m_setting->m_keyboard.addKey(key);
    if (m_ast) {
        m_ast->keyDefinitions.push_back(key_def);
    }
}


// <DEFINE_MODIFIER>
void SettingLoader::load_DEFINE_MODIFIER()
{
    yamy::ast::ModifierDefinition mod_def;
    Token *t = getToken();
    Modifier::Type mt;
    if      (*t == "shift"  ) { mt = Modifier::Type_Shift; mod_def.type = "shift"; }
    else if (*t == "alt"     ||
             *t == "meta"    ||
             *t == "menu"   ) { mt = Modifier::Type_Alt; mod_def.type = "alt"; }
    else if (*t == "control" ||
             *t == "ctrl"   ) { mt = Modifier::Type_Control; mod_def.type = "control"; }
    else if (*t == "windows" ||
             *t == "win"    ) { mt = Modifier::Type_Windows; mod_def.type = "windows"; }
    else throw ErrorMessage() << "`" << *t
        << "': invalid modifier name.";

    if (*getToken() != "=")
        throw ErrorMessage() << "there must be `=' after modifier name.";

    while (!isEOL()) {
        t = getToken();
        mod_def.keyNames.push_back(t->getString());
        Key *key =
            m_setting->m_keyboard.searchKeyByNonAliasName(t->getString());
        if (!key)
            throw ErrorMessage() << "`" << *t << "': invalid key name.";
        m_setting->m_keyboard.addModifier(mt, key);
    }

    if (m_ast) {
        m_ast->modifierDefinitions.push_back(mod_def);
    }
}


// <DEFINE_SYNC_KEY>
void SettingLoader::load_DEFINE_SYNC_KEY()
{
    Key *key = m_setting->m_keyboard.getSyncKey();
    key->initialize();
    key->addName("sync");

    if (*getToken() != "=")
        throw ErrorMessage() << "there must be `=' after `sync'.";
    
    yamy::ast::KeyDefinition key_def;
    key_def.names.push_back("sync");

    load_SCAN_CODES(key, &key_def.scanCodes);

    if (m_ast) {
        m_ast->keyDefinitions.push_back(key_def);
    }
}


// <DEFINE_ALIAS>
void SettingLoader::load_DEFINE_ALIAS()
{
    yamy::ast::AliasDefinition alias_def;
    Token *name = getToken();
    alias_def.aliasName = name->getString();

    if (*getToken() != "=")
        throw ErrorMessage() << "there must be `=' after `alias'.";

    Token *t = getToken();
    alias_def.keyName = t->getString();
    Key *key = m_setting->m_keyboard.searchKeyByNonAliasName(t->getString());
    if (!key)
        throw ErrorMessage() << "`" << *t << "': invalid key name.";
    m_setting->m_keyboard.addAlias(name->getString(), key);
    if (m_ast) {
        m_ast->aliasDefinitions.push_back(alias_def);
    }
}


// <DEFINE_SUBSTITUTE>
void SettingLoader::load_DEFINE_SUBSTITUTE()
{
    typedef std::list<ModifiedKey> AssignedKeys;
    AssignedKeys assignedKeys;
    do {
        ModifiedKey mkey;
        mkey.m_modifier =
            load_MODIFIER(Modifier::Type_ASSIGN, m_defaultAssignModifier);
        mkey.m_key = load_KEY_NAME();
        assignedKeys.push_back(mkey);
    } while (!(*lookToken() == "=>" || *lookToken() == "="));
    getToken();

    KeySeq *keySeq = load_KEY_SEQUENCE("", false, Modifier::Type_ASSIGN);
    ModifiedKey mkey = keySeq->getFirstModifiedKey();
    if (!mkey.m_key)
        throw ErrorMessage() << "no key is specified for substitute.";

    for (AssignedKeys::iterator i = assignedKeys.begin();
            i != assignedKeys.end(); ++ i)
        m_setting->m_keyboard.addSubstitute(*i, mkey);
}


// <DEFINE_NUMBER_MODIFIER>
// Syntax: def numbermod *_1 = *LShift
void SettingLoader::load_DEFINE_NUMBER_MODIFIER()
{
    yamy::ast::NumberModifierDefinition num_mod_def;
    // Get the number key name (e.g., *_1)
    Token *numberKeyToken = getToken();
    std::string numberKeyName = numberKeyToken->getString();
    num_mod_def.numberKeyName = numberKeyName;

    // Look up the number key
    Key *numberKey = m_setting->m_keyboard.searchKeyByNonAliasName(numberKeyName);
    if (!numberKey)
        throw ErrorMessage() << "`" << numberKeyName << "': invalid number key name.";

    // Expect '='
    if (*getToken() != "=")
        throw ErrorMessage() << "there must be `=' after number key name in `def numbermod'.";

    // Get the modifier key name (e.g., *LShift)
    Token *modifierKeyToken = getToken();
    std::string modifierKeyName = modifierKeyToken->getString();
    num_mod_def.modifierKeyName = modifierKeyName;

    // Validate that it's a valid hardware modifier
    static const char* validModifiers[] = {
        "LShift", "RShift", "LCtrl", "RCtrl",
        "LAlt", "RAlt", "LWin", "RWin"
    };
    bool isValidModifier = false;
    for (const char* validMod : validModifiers) {
        if (strcasecmp_utf8(modifierKeyName.c_str(), validMod) == 0) {
            isValidModifier = true;
            break;
        }
    }
    if (!isValidModifier) {
        ErrorMessage e;
        e << "`" << modifierKeyName << "': invalid modifier key. ";
        e << "Valid modifiers: LShift, RShift, LCtrl, RCtrl, LAlt, RAlt, LWin, RWin.";
        throw e;
    }

    // Look up the modifier key
    Key *modifierKey = m_setting->m_keyboard.searchKeyByNonAliasName(modifierKeyName);
    if (!modifierKey)
        throw ErrorMessage() << "`" << modifierKeyName << "': invalid modifier key name.";

    // Add the number modifier mapping
    m_setting->m_keyboard.addNumberModifier(numberKey, modifierKey);

    if (m_ast) {
        m_ast->numberModifierDefinitions.push_back(num_mod_def);
    }
}


// <DEFINE_OPTION>
void SettingLoader::load_DEFINE_OPTION()
{
    Token *t = getToken();
    if (*t == "KL-") {
        if (*getToken() != "=") {
            throw ErrorMessage() << "there must be `=' after `def option KL-'.";
        }

        load_ARGUMENT(&m_setting->m_correctKanaLockHandling);

    } else if (*t == "delay-of") {
        if (*getToken() != "!!!") {
            throw ErrorMessage()
            << "there must be `!!!' after `def option delay-of'.";
        }

        if (*getToken() != "=") {
            throw ErrorMessage()
            << "there must be `=' after `def option delay-of !!!'.";
        }

        load_ARGUMENT(&m_setting->m_oneShotRepeatableDelay);

    } else if (*t == "sts4mayu") {
        if (*getToken() != "=") {
            throw ErrorMessage()
            << "there must be `=' after `def option sts4mayu'.";
        }

        load_ARGUMENT(&m_setting->m_sts4mayu);

    } else if (*t == "cts4mayu") {
        if (*getToken() != "=") {
            throw ErrorMessage()
            << "there must be `=' after `def option cts4mayu'.";
        }

        load_ARGUMENT(&m_setting->m_cts4mayu);

    } else if (*t == "mouse-event") {
        if (*getToken() != "=") {
            throw ErrorMessage()
            << "there must be `=' after `def option mouse-event'.";
        }

        load_ARGUMENT(&m_setting->m_mouseEvent);

    } else if (*t == "drag-threshold") {
        if (*getToken() != "=") {
            throw ErrorMessage()
            << "there must be `=' after `def option drag-threshold'.";
        }

        load_ARGUMENT(&m_setting->m_dragThreshold);

    } else {
        throw ErrorMessage() << "syntax error `def option " << *t << "'.";
    }
}



// <KEYBOARD_DEFINITION>
void SettingLoader::load_KEYBOARD_DEFINITION()
{
    Token *t = getToken();

    // <DEFINE_KEY>
    if (*t == "key") load_DEFINE_KEY();

    // <DEFINE_MODIFIER>
    else if (*t == "mod") load_DEFINE_MODIFIER();

    // <DEFINE_SYNC_KEY>
    else if (*t == "sync") load_DEFINE_SYNC_KEY();

    // <DEFINE_ALIAS>
    else if (*t == "alias") load_DEFINE_ALIAS();

    // <DEFINE_SUBSTITUTE>
    else if (*t == "subst") load_DEFINE_SUBSTITUTE();

    // <DEFINE_NUMBER_MODIFIER>
    else if (*t == "numbermod") load_DEFINE_NUMBER_MODIFIER();

    // <DEFINE_OPTION>
    else if (*t == "option") load_DEFINE_OPTION();

    //
    else throw ErrorMessage() << "syntax error `" << *t << "'.";
}

// Helper to parse hex digits
static int parseHex2(const std::string& s) {
    auto hexToInt = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return 0;
    };
    return (hexToInt(s[1]) << 4) | hexToInt(s[2]);
}

bool SettingLoader::parseMxxModifier(const std::string& token_str, Modifier::Type, Modifier&, Modifier::Type*, Modifier&, int& flag)
{
    if (token_str.length() != 4 || token_str[0] != 'M' || token_str[3] != '-') return false;
    
    // Validate hex digits
    auto isHexDigit = [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    };
    if (!isHexDigit(token_str[1]) || !isHexDigit(token_str[2])) return false;

    int modNum = parseHex2(token_str);
    getToken(); // Consume token

    std::cerr << "[PARSER:NEW] Parsed M" << std::hex << std::setw(2) << std::setfill('0') << modNum
              << "- -> virtual modifier " << std::dec << (int)modNum
              << " (NEW M00-MFF system)" << std::endl;

    m_parserContext.pendingVirtualMod = static_cast<uint8_t>(modNum);
    m_parserContext.hasVirtualMod = true;

    flag = 0; // PRESS
    return true;
}

bool SettingLoader::parseLxxModifier(const std::string& token_str, Modifier::Type i_mode, Modifier& i_modifier, Modifier::Type* o_mode, Modifier& isModifierSpecified, int& flag)
{
    if (token_str.length() != 4 || token_str[0] != 'L' || token_str[3] != '-') return false;

    auto isHexDigit = [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    };
    if (!isHexDigit(token_str[1]) || !isHexDigit(token_str[2])) return false;

    int modNum = parseHex2(token_str);

    if (modNum < 10) {
        getToken(); // Consume token
        Modifier::Type mt = static_cast<Modifier::Type>(Modifier::Type_Lock0 + modNum);
        if (static_cast<int>(i_mode) <= static_cast<int>(mt))
            throw ErrorMessage() << "`" << token_str << "': invalid modifier at this context.";
        
        switch (flag) {
            case 0: /*PRESS*/    i_modifier.press(mt); break;
            case 1: /*RELEASE*/  i_modifier.release(mt); break;
            case 2: /*DONTCARE*/ i_modifier.dontcare(mt); break;
        }
        isModifierSpecified.on(mt);
        flag = 0; // PRESS

        if (o_mode && *o_mode < mt) {
            if (mt < Modifier::Type_BASIC)
                *o_mode = Modifier::Type_BASIC;
            else if (mt < Modifier::Type_KEYSEQ)
                *o_mode = Modifier::Type_KEYSEQ;
            else if (mt < Modifier::Type_ASSIGN)
                *o_mode = Modifier::Type_ASSIGN;
        }
        return true;
    } else {
        throw ErrorMessage() << "`" << token_str << "': L10-LFF not yet implemented. Use L00-L09 for now.";
    }
}


// <..._MODIFIER>
Modifier SettingLoader::load_MODIFIER(
    Modifier::Type i_mode, Modifier i_modifier, Modifier::Type *o_mode)
{
    if (o_mode)
        *o_mode = Modifier::Type_begin;

    Modifier isModifierSpecified;
    enum { PRESS, RELEASE, DONTCARE } flag = PRESS;

    int i;
    for (i = i_mode; i < Modifier::Type_ASSIGN; ++ i) {
        i_modifier.dontcare(Modifier::Type(i));
        isModifierSpecified.on(Modifier::Type(i));
    }

    Token *t = nullptr;

continue_loop:
    while (!isEOL()) {
        t = lookToken();

        const static struct {
            const char *m_s;
            Modifier::Type m_mt;
        } map[] = {
            // <BASIC_MODIFIER>
            { "S-",  Modifier::Type_Shift },
            { "A-",  Modifier::Type_Alt },
            { "M-",  Modifier::Type_Alt },
            { "C-",  Modifier::Type_Control },
            { "W-",  Modifier::Type_Windows },
            // <KEYSEQ_MODIFIER>
            { "U-",  Modifier::Type_Up },
            { "D-",  Modifier::Type_Down },
            // <ASSIGN_MODIFIER>
            { "R-",  Modifier::Type_Repeat },
            { "IL-", Modifier::Type_ImeLock },
            { "IC-", Modifier::Type_ImeComp },
            { "I-",  Modifier::Type_ImeComp },
            { "NL-", Modifier::Type_NumLock },
            { "CL-", Modifier::Type_CapsLock },
            { "SL-", Modifier::Type_ScrollLock },
            { "KL-", Modifier::Type_KanaLock },
            { "MAX-", Modifier::Type_Maximized },
            { "MIN-", Modifier::Type_Minimized },
            { "MMAX-", Modifier::Type_MdiMaximized },
            { "MMIN-", Modifier::Type_MdiMinimized },
            { "T-", Modifier::Type_Touchpad },
            { "TS-", Modifier::Type_TouchpadSticky },
            { "L0-", Modifier::Type_Lock0 },
            { "L1-", Modifier::Type_Lock1 },
            { "L2-", Modifier::Type_Lock2 },
            { "L3-", Modifier::Type_Lock3 },
            { "L4-", Modifier::Type_Lock4 },
            { "L5-", Modifier::Type_Lock5 },
            { "L6-", Modifier::Type_Lock6 },
            { "L7-", Modifier::Type_Lock7 },
            { "L8-", Modifier::Type_Lock8 },
            { "L9-", Modifier::Type_Lock9 },
        };

        // NEW: Dynamic M00-MFF and L00-LFF parsing
        if (parseMxxModifier(token_str, i_mode, i_modifier, o_mode, isModifierSpecified, (int&)flag)) {
            continue;
        }
        if (parseLxxModifier(token_str, i_mode, i_modifier, o_mode, isModifierSpecified, (int&)flag)) {
            continue;
        }

        for (int i = 0; i < (int)NUMBER_OF(map); ++ i)
            if (*t == map[i].m_s) {
                getToken();
                Modifier::Type mt = map[i].m_mt;
                if (static_cast<int>(i_mode) <= static_cast<int>(mt))
                    throw ErrorMessage() << "`" << *t
                    << "': invalid modifier at this context.";
                switch (flag) {
                case PRESS:
                    i_modifier.press(mt);
                    break;
                case RELEASE:
                    i_modifier.release(mt);
                    break;
                case DONTCARE:
                    i_modifier.dontcare(mt);
                    break;
                }
                isModifierSpecified.on(mt);
                flag = PRESS;

                if (o_mode && *o_mode < mt) {
                    if (mt < Modifier::Type_BASIC)
                        *o_mode = Modifier::Type_BASIC;
                    else if (mt < Modifier::Type_KEYSEQ)
                        *o_mode = Modifier::Type_KEYSEQ;
                    else if (mt < Modifier::Type_ASSIGN)
                        *o_mode = Modifier::Type_ASSIGN;
                }
                goto continue_loop;
            }

        if (*t == "*") {
            getToken();
            flag = DONTCARE;
            continue;
        }

        if (*t == "~") {
            getToken();
            flag = RELEASE;
            continue;
        }

        break;
    }

    for (i = Modifier::Type_begin; i != Modifier::Type_end; ++ i)
        if (!isModifierSpecified.isOn(Modifier::Type(i)))
            switch (flag) {
            case PRESS:
                break;
            case RELEASE:
                i_modifier.release(Modifier::Type(i));
                break;
            case DONTCARE:
                i_modifier.dontcare(Modifier::Type(i));
                break;
            }

    // fix up and down
    bool isDontcareUp   = i_modifier.isDontcare(Modifier::Type_Up);
    bool isDontcareDown = i_modifier.isDontcare(Modifier::Type_Down);
    bool isOnUp         = i_modifier.isOn(Modifier::Type_Up);
    bool isOnDown       = i_modifier.isOn(Modifier::Type_Down);
    if (isDontcareUp && isDontcareDown)
        ;
    else if (isDontcareUp)
        i_modifier.on(Modifier::Type_Up, !isOnDown);
    else if (isDontcareDown)
        i_modifier.on(Modifier::Type_Down, !isOnUp);
    else if (isOnUp == isOnDown) {
        i_modifier.dontcare(Modifier::Type_Up);
        i_modifier.dontcare(Modifier::Type_Down);
    }

    // fix repeat
    if (!isModifierSpecified.isOn(Modifier::Type_Repeat))
        i_modifier.dontcare(Modifier::Type_Repeat);
    return i_modifier;
}


// Create a virtual key with a given keycode
// Virtual keys use a special scan code format to store the keycode
Key *SettingLoader::createVirtualKey(const std::string &i_name, uint16_t i_keycode)
{
    // Create a new key with the virtual name
    Key key;
    key.addName(i_name);

    // Store the keycode as a special scan code
    // We use a unique pattern: scan = keycode, flags = 0xFFFF (marker for virtual)
    ScanCode virtualSc(i_keycode, 0xFFFF);
    key.addScanCode(virtualSc);

    // Add the key to the keyboard
    m_setting->m_keyboard.addKey(key);

    // Return pointer to the newly added key by searching for it
    Key *newKey = m_setting->m_keyboard.searchKey(i_name);
    if (!newKey)
        throw ErrorMessage() << "Failed to add virtual key `" << i_name << "'.";
    return newKey;
}

// <KEY_NAME>
Key *SettingLoader::load_KEY_NAME()
{
    Token *t = getToken();
    std::string keyName = t->getString();

    // Check for virtual key prefix (V_*)
    if (keyName.length() > 2 && keyName[0] == 'V' && keyName[1] == '_') {
        // Extract base key name after "V_"
        std::string baseKeyName = keyName.substr(2);
        Key *baseKey = m_setting->m_keyboard.searchKey(baseKeyName);
        if (!baseKey)
            throw ErrorMessage() << "`" << keyName << "': invalid base key name after V_.";

        // Get base key's first scan code to determine offset
        const ScanCode *baseScanCodes = baseKey->getScanCodes();
        if (baseKey->getScanCodesSize() == 0)
            throw ErrorMessage() << "`" << keyName << "': base key has no scan codes.";

        // Calculate virtual key code: VIRTUAL_KEY_BASE + base_offset
        uint16_t baseOffset = baseScanCodes[0].m_scan;
        uint16_t virtualKeyCode = 0xE000 + baseOffset; // YAMY_VIRTUAL_KEY_BASE + offset

        // Check if this virtual key already exists
        Key *existingKey = m_setting->m_keyboard.searchKey(keyName);
        if (existingKey)
            return existingKey;

        // Create and return new virtual key
        return createVirtualKey(keyName, virtualKeyCode);
    }

    // Check for modal modifier pattern (M00-MFF)
    if (keyName.length() == 3 && keyName[0] == 'M' &&
        std::isxdigit(keyName[1]) && std::isxdigit(keyName[2])) {
        // Parse hex digits (case insensitive)
        char hex1 = std::toupper(keyName[1]);
        char hex2 = std::toupper(keyName[2]);

        // Validate hex digits
        if (!((hex1 >= '0' && hex1 <= '9') || (hex1 >= 'A' && hex1 <= 'F')) ||
            !((hex2 >= '0' && hex2 <= '9') || (hex2 >= 'A' && hex2 <= 'F'))) {
            throw ErrorMessage() << "`" << keyName << "': invalid hex digits in modifier name.";
        }

        // Convert hex chars to number
        int modNum = (hex1 >= 'A' ? (hex1 - 'A' + 10) : (hex1 - '0')) * 16 +
                     (hex2 >= 'A' ? (hex2 - 'A' + 10) : (hex2 - '0'));

        // Create virtual keycode for modifier: YAMY_MOD_00 + modNum
        uint16_t modKeyCode = 0xF000 + modNum;

        // Check if this modifier key already exists
        Key *existingKey = m_setting->m_keyboard.searchKey(keyName);
        if (existingKey)
            return existingKey;

        // Create and return new modifier key
        return createVirtualKey(keyName, modKeyCode);
    }

    // Check for lock key pattern (L00-LFF)
    if (keyName.length() == 3 && keyName[0] == 'L' &&
        std::isxdigit(keyName[1]) && std::isxdigit(keyName[2])) {
        // Parse hex digits (case insensitive)
        char hex1 = std::toupper(keyName[1]);
        char hex2 = std::toupper(keyName[2]);

        // Validate hex digits
        if (!((hex1 >= '0' && hex1 <= '9') || (hex1 >= 'A' && hex1 <= 'F')) ||
            !((hex2 >= '0' && hex2 <= '9') || (hex2 >= 'A' && hex2 <= 'F'))) {
            throw ErrorMessage() << "`" << keyName << "': invalid hex digits in lock name.";
        }

        // Convert hex chars to number
        int lockNum = (hex1 >= 'A' ? (hex1 - 'A' + 10) : (hex1 - '0')) * 16 +
                      (hex2 >= 'A' ? (hex2 - 'A' + 10) : (hex2 - '0'));

        // Create virtual keycode for lock: YAMY_LOCK_00 + lockNum
        uint16_t lockKeyCode = 0xF100 + lockNum;

        // Check if this lock key already exists
        Key *existingKey = m_setting->m_keyboard.searchKey(keyName);
        if (existingKey)
            return existingKey;

        // Create and return new lock key
        return createVirtualKey(keyName, lockKeyCode);
    }

    // Standard key lookup
    Key *key = m_setting->m_keyboard.searchKey(t->getString());
    if (!key)
        throw ErrorMessage() << "`" << *t << "': invalid key name.";
    return key;
}


// <KEYMAP_DEFINITION>
void SettingLoader::load_KEYMAP_DEFINITION(const Token *i_which)
{
    Keymap::Type type = Keymap::Type_keymap;
    Token *name = getToken();    // <KEYMAP_NAME>
    std::string windowClassName;
    std::string windowTitleName;
    KeySeq *keySeq = nullptr;
    Keymap *parentKeymap = nullptr;
    bool isKeymap2 = false;
    bool doesLoadDefaultKeySeq = false;

    if (!isEOL()) {
        Token *t = lookToken();
        if (*i_which == "window") {    // <WINDOW>
            if (t->isOpenParen())
                // "(" <WINDOW_CLASS_NAME> "&&" <WINDOW_TITLE_NAME> ")"
                // "(" <WINDOW_CLASS_NAME> "||" <WINDOW_TITLE_NAME> ")"
            {
                getToken();
                windowClassName = getToken()->getRegexp();
                t = getToken();
                if (*t == "&&")
                    type = Keymap::Type_windowAnd;
                else if (*t == "||")
                    type = Keymap::Type_windowOr;
                else
                    throw ErrorMessage() << "`" << *t << "': unknown operator.";
                windowTitleName = getToken()->getRegexp();
                if (!getToken()->isCloseParen())
                    throw ErrorMessage() << "there must be `)'.";
            } else if (t->isRegexp()) {    // <WINDOW_CLASS_NAME>
                getToken();
                type = Keymap::Type_windowAnd;
                windowClassName = t->getRegexp();
            }
        } else if (*i_which == "keymap")
            ;
        else if (*i_which == "keymap2")
            isKeymap2 = true;
        else
            ASSERT(false);

        if (!isEOL())
            doesLoadDefaultKeySeq = true;
    }

    m_currentKeymap = m_setting->m_keymaps.add(
                          Keymap(type, to_UTF_8(name->getString()), windowClassName, windowTitleName,
                                 nullptr, nullptr));
    
    // AST population
    yamy::ast::KeymapDefinition keymapDef;
    keymapDef.name = to_UTF_8(name->getString());
    keymapDef.windowClassRegex = windowClassName;
    keymapDef.windowTitleRegex = windowTitleName;
    m_ast->keymaps.push_back(keymapDef);
    m_currentAstKeymap = &m_ast->keymaps.back();

    if (doesLoadDefaultKeySeq) {
        Token *t = lookToken();
        // <KEYMAP_PARENT>
        if (*t == ":") {
            getToken();
            t = getToken();
            parentKeymap = m_setting->m_keymaps.searchByName(to_UTF_8(t->getString()));
            if (!parentKeymap)
                throw ErrorMessage() << "`" << *t
                << "': unknown keymap name.";
            if (m_currentAstKeymap) {
                m_currentAstKeymap->parentName = to_UTF_8(t->getString());
            }
        }
        if (!isEOL()) {
            t = getToken();
            if (!(*t == "=>" || *t == "="))
                throw ErrorMessage() << "`" << *t << "': syntax error.";
            keySeq = SettingLoader::load_KEY_SEQUENCE();
        }
    }
    if (keySeq == nullptr) {
        FunctionData *fd;
        if (type == Keymap::Type_keymap && !isKeymap2)
            fd = createFunctionData("KeymapParent");
        else if (type == Keymap::Type_keymap && !isKeymap2)
            fd = createFunctionData("Undefined");
        else // (type == Keymap::Type_windowAnd || type == Keymap::Type_windowOr)
            fd = createFunctionData("KeymapParent");
        ASSERT( fd );
        keySeq = m_setting->m_keySeqs.add(
                     KeySeq(to_UTF_8(name->getString())).add(ActionFunction(fd)));
    }

    m_currentKeymap->setIfNotYet(keySeq, parentKeymap);
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(bool *o_arg)
{
    *o_arg = !(*getToken() == "false");
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(int *o_arg)
{
    *o_arg = getToken()->getNumber();
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(unsigned int *o_arg)
{
    *o_arg = getToken()->getNumber();
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(long *o_arg)
{
    *o_arg = getToken()->getNumber();
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(unsigned long *o_arg)
{
    *o_arg = getToken()->getNumber();
}


#ifdef _WIN32
// &lt;ARGUMENT&gt; - Windows only (on Linux, long and __int64 are the same)
void SettingLoader::load_ARGUMENT(unsigned __int64 *o_arg)
{
    *o_arg = getToken()->getNumber();
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(__int64 *o_arg)
{
    *o_arg = getToken()->getNumber();
}
#endif


#ifdef _WIN32
// Windows: tstring is wstring, so we need a separate version for std::string
// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(tstring *o_arg)
{
    *o_arg = to_tstring(getToken()->getString());
}
#endif

// <ARGUMENT>
void SettingLoader::load_ARGUMENT(std::string *o_arg)
{
#ifdef _UNICODE
    *o_arg = to_UTF_8(getToken()->getString());
#else
    // On Linux, getString() returns std::string directly
    *o_arg = getToken()->getString();
#endif
}

#ifdef _WIN32
// Windows: tstring is wstring, so we need a separate version for std::string
// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(std::list<tstring> *o_arg)
{
    while (true) {
        if (!lookToken()->isString())
            return;
        o_arg->push_back(to_tstring(getToken()->getString()));

        if (!lookToken()->isComma())
            return;
        getToken();
    }
}
#endif

// <ARGUMENT>
void SettingLoader::load_ARGUMENT(std::list<std::string> *o_arg)
{
    while (true) {
        if (!lookToken()->isString())
            return;
        std::string s;
#ifdef _UNICODE
        s = to_UTF_8(getToken()->getString());
#else
        // On Linux, getString() returns std::string directly
        s = getToken()->getString();
#endif
        o_arg->push_back(s);

        if (!lookToken()->isComma())
            return;
        getToken();
    }
}


// <ARGUMENT>
void SettingLoader::load_ARGUMENT(Regex *o_arg)
{
    std::string pattern = getToken()->getRegexp();
    *o_arg = Regex(pattern);
}
// Note: tregex is an alias for Regex, so no separate overload needed


// &lt;ARGUMENT_VK&gt;
void SettingLoader::load_ARGUMENT(VKey *o_arg)
{
    Token *t = getToken();
    int vkey = 0;
    while (true) {
        if (t->isNumber()) {
            vkey |= static_cast<BYTE>(t->getNumber());
            break;
        } else if (*t == "E-") vkey |= VKey_extended;
        else if (*t == "U-") vkey |= VKey_released;
        else if (*t == "D-") vkey |= VKey_pressed;
        else {
            const VKeyTable *vkt;
            for (vkt = g_vkeyTable; vkt->m_name; ++ vkt)
                if (*t == vkt->m_name)
                    break;
            if (!vkt->m_name)
                throw ErrorMessage() << "`" << *t
                << "': unknown virtual key name.";
            vkey |= vkt->m_code;
            break;
        }
        t = getToken();
    }
    if (!(vkey & VKey_released) && !(vkey & VKey_pressed))
        vkey |= VKey_released | VKey_pressed;
    *o_arg = static_cast<VKey>(vkey);
}


// &lt;ARGUMENT_WINDOW&gt;
void SettingLoader::load_ARGUMENT(ToWindowType *o_arg)
{
    Token *t = getToken();
    if (t->isNumber()) {
        if (ToWindowType_toBegin <= t->getNumber()) {
            *o_arg = static_cast<ToWindowType>(t->getNumber());
            return;
        }
    } else if (getTypeValue(o_arg, t->getString()))
        return;
    throw ErrorMessage() << "`" << *t << "': invalid target window.";
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(GravityType *o_arg)
{
    Token *t = getToken();
    if (getTypeValue(o_arg, t->getString()))
        return;
    throw ErrorMessage() << "`" << *t << "': unknown gravity symbol.";
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(MouseHookType *o_arg)
{
    Token *t = getToken();
    if (getTypeValue(o_arg, t->getString()))
        return;
    throw ErrorMessage() << "`" << *t << "': unknown MouseHookType symbol.";
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(MayuDialogType *o_arg)
{
    Token *t = getToken();
    if (getTypeValue(o_arg, t->getString()))
        return;
    throw ErrorMessage() << "`" << *t << "': unknown dialog box.";
}


// &lt;ARGUMENT_LOCK&gt;
void SettingLoader::load_ARGUMENT(ModifierLockType *o_arg)
{
    Token *t = getToken();
    if (getTypeValue(o_arg, t->getString()))
        return;
    throw ErrorMessage() << "`" << *t << "': unknown lock name.";
}


// &lt;ARGUMENT_LOCK&gt;
void SettingLoader::load_ARGUMENT(ToggleType *o_arg)
{
    Token *t = getToken();
    if (getTypeValue(o_arg, t->getString()))
        return;
    throw ErrorMessage() << "`" << *t << "': unknown toggle name.";
}


// &lt;ARGUMENT_SHOW_WINDOW&gt;
void SettingLoader::load_ARGUMENT(ShowCommandType *o_arg)
{
    Token *t = getToken();
    if (getTypeValue(o_arg, t->getString()))
        return;
    throw ErrorMessage() << "`" << *t << "': unknown show command.";
}


// &lt;ARGUMENT_TARGET_WINDOW&gt;
void SettingLoader::load_ARGUMENT(TargetWindowType *o_arg)
{
    Token *t = getToken();
    if (getTypeValue(o_arg, t->getString()))
        return;
    throw ErrorMessage() << "`" << *t
    << "': unknown target window type.";
}


// &lt;bool&gt;
void SettingLoader::load_ARGUMENT(BooleanType *o_arg)
{
    Token *t = getToken();
    if (getTypeValue(o_arg, t->getString()))
        return;
    throw ErrorMessage() << "`" << *t << "': must be true or false.";
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(LogicalOperatorType *o_arg)
{
    Token *t = getToken();
    if (getTypeValue(o_arg, t->getString()))
        return;
    throw ErrorMessage() << "`" << *t << "': must be 'or' or 'and'.";
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(Modifier *o_arg)
{
    Modifier modifier;
    for (int i = Modifier::Type_begin; i != Modifier::Type_end; ++ i)
        modifier.dontcare(static_cast<Modifier::Type>(i));
    *o_arg = load_MODIFIER(Modifier::Type_ASSIGN, modifier);
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(const Keymap **o_arg)
{
    Token *t = getToken();
    const Keymap *&keymap = *o_arg;
    keymap = m_setting->m_keymaps.searchByName(to_UTF_8(t->getString()));
    if (!keymap)
        throw ErrorMessage() << "`" << *t << "': unknown keymap name.";
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(const KeySeq **o_arg)
{
    Token *t = getToken();
    const KeySeq *&keySeq = *o_arg;
    if (t->isOpenParen()) {
        keySeq = load_KEY_SEQUENCE("", true);
        getToken(); // close paren
    } else if (*t == "$") {
        t = getToken();
        keySeq = m_setting->m_keySeqs.searchByName(to_UTF_8(t->getString()));
        if (!keySeq)
            throw ErrorMessage() << "`$" << *t << "': unknown keyseq name.";
    } else
        throw ErrorMessage() << "`" << *t << "': it is not keyseq.";
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(StrExprArg *o_arg)
{
    Token *t = getToken();
    StrExprArg::Type type = StrExprArg::Literal;
    if (*t == "$" && t->isQuoted() == false
            && lookToken()->getType() == Token::Type_string) {
        type = StrExprArg::Builtin;
        t = getToken();
    }
    *o_arg = StrExprArg(to_UTF_8(t->getString()), type);
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(WindowMonitorFromType *o_arg)
{
    Token *t = getToken();
    if (getTypeValue(o_arg, t->getString()))
        return;
    throw ErrorMessage() << "`" << *t
    << "': unknown monitor from type.";
}


// <KEY_SEQUENCE>
KeySeq *SettingLoader::load_KEY_SEQUENCE(
    const std::string &i_name, bool i_isInParen, Modifier::Type i_mode)
{
    KeySeq keySeq(i_name);
    while (!isEOL()) {
        Modifier::Type mode;
        Modifier modifier = load_MODIFIER(i_mode, m_defaultKeySeqModifier, &mode);
        keySeq.setMode(mode);
        Token *t = lookToken();
        if (t->isCloseParen() && i_isInParen)
            break;
        else if (t->isOpenParen()) {
            getToken(); // open paren
            KeySeq *ks = load_KEY_SEQUENCE("", true, i_mode);
            getToken(); // close paren
            keySeq.add(ActionKeySeq(ks));
        } else if (*t == "$") { // <KEYSEQ_NAME>
            getToken();
            t = getToken();
            KeySeq *ks = m_setting->m_keySeqs.searchByName(to_UTF_8(t->getString()));
            if (ks == nullptr)
                throw ErrorMessage() << "`$" << *t
                << "': unknown keyseq name.";
            if (!ks->isCorrectMode(i_mode))
                throw ErrorMessage()
                << "`$" << *t
                << "': Some of R-, IL-, IC-, NL-, CL-, SL-, KL-, MAX-, MIN-, MMAX-, MMIN-, T-, TS-, M0...M19- and L0...L9- are used in the keyseq.  They are prohibited in this context.";
            keySeq.setMode(ks->getMode());
            keySeq.add(ActionKeySeq(ks));
        } else if (*t == "&") { // <FUNCTION_NAME>
            getToken();
            t = getToken();

            // search function
            ActionFunction af(createFunctionData(t->getString()), modifier);
            if (af.m_functionData == nullptr)
                throw ErrorMessage() << "`&" << *t
                << "': unknown function name.";
            af.m_functionData->load(this);
            keySeq.add(af);
        } else { // <KEYSEQ_MODIFIED_KEY_NAME>
            ModifiedKey mkey;
            mkey.m_modifier = modifier;
            mkey.m_key = load_KEY_NAME();

            // NEW M00-MFF system: Set virtual modifier if pending
            if (m_parserContext.hasVirtualMod) {
                mkey.setVirtualMod(m_parserContext.pendingVirtualMod, true);
                std::cerr << "[PARSER:NEW] Set virtual mod M" << std::hex << std::setw(2) << std::setfill('0') << (int)m_parserContext.pendingVirtualMod
                          << " in ModifiedKey for key " << (mkey.m_key ? mkey.m_key->getName() : "NULL") << std::dec << std::endl;
                m_parserContext.hasVirtualMod = false; // Reset after use
            }

            keySeq.add(ActionKey(mkey));
        }
    }
    return m_setting->m_keySeqs.add(keySeq);
}


// <KEY_ASSIGN>
void SettingLoader::load_KEY_ASSIGN()
{
    typedef std::list<ModifiedKey> AssignedKeys;
    AssignedKeys assignedKeys;

    ModifiedKey mkey;
    mkey.m_modifier =
        load_MODIFIER(Modifier::Type_ASSIGN, m_defaultAssignModifier);
    if (*lookToken() == "=") {
        getToken();
        m_defaultKeySeqModifier = load_MODIFIER(Modifier::Type_KEYSEQ,
                                                m_defaultKeySeqModifier);
        m_defaultAssignModifier = mkey.m_modifier;
        return;
    }

    while (true) {
        mkey.m_key = load_KEY_NAME();
        assignedKeys.push_back(mkey);
        if (*lookToken() == "=>" || *lookToken() == "=")
            break;
        mkey.m_modifier =
            load_MODIFIER(Modifier::Type_ASSIGN, m_defaultAssignModifier);
    }
    getToken();

    ASSERT(m_currentKeymap);
    KeySeq *keySeq = load_KEY_SEQUENCE();
    for (AssignedKeys::iterator i = assignedKeys.begin();
            i != assignedKeys.end(); ++ i)
        m_currentKeymap->addAssignment(*i, keySeq);
}


// <EVENT_ASSIGN>
void SettingLoader::load_EVENT_ASSIGN()
{
    std::list<ModifiedKey> assignedKeys;

    ModifiedKey mkey;
    mkey.m_modifier.dontcare();            //set all modifiers to dontcare

    Token *t = getToken();
    Key **e;
    for (e = Event::events; *e; ++ e)
        if (*t == (*e)->getName()) {
            mkey.m_key = *e;
            break;
        }
    if (!*e)
        throw ErrorMessage() << "`" << *t << "': invalid event name.";

    t = getToken();
    if (!(*t == "=>" || *t == "="))
        throw ErrorMessage() << "`=' is expected.";

    ASSERT(m_currentKeymap);
    KeySeq *keySeq = load_KEY_SEQUENCE();
    m_currentKeymap->addAssignment(mkey, keySeq);
}


// <MOD_ASSIGN> - Parse "mod assign M00 = *Enter" syntax for tap actions
void SettingLoader::load_MOD_ASSIGN()
{
    // Expect "assign" token (already consumed "mod" in load_LINE)
    Token *t = getToken();
    if (*t != "assign")
        throw ErrorMessage() << "expected 'assign' after 'mod', got `" << *t << "'.";

    // Get modifier key (M00-MFF)
    t = getToken();
    std::string modName = t->getString();

    // Validate modifier key format (M00-MFF)
    if (modName.length() != 3 || modName[0] != 'M' ||
        !std::isxdigit(modName[1]) || !std::isxdigit(modName[2])) {
        throw ErrorMessage() << "`" << modName << "': invalid modifier format. Expected M00-MFF.";
    }

    // Parse hex digits to get modifier number
    char hex1 = std::toupper(modName[1]);
    char hex2 = std::toupper(modName[2]);
    int modNum = (hex1 >= 'A' ? (hex1 - 'A' + 10) : (hex1 - '0')) * 16 +
                 (hex2 >= 'A' ? (hex2 - 'A' + 10) : (hex2 - '0'));

    // Expect '='
    t = getToken();
    if (*t != "=")
        throw ErrorMessage() << "expected '=' after modifier name, got `" << *t << "'.";

    // Expect '*' prefix for key
    t = getToken();
    if (*t != "*")
        throw ErrorMessage() << "expected '*' before tap output key, got `" << *t << "'.";

    // Get tap output key
    Key *tapKey = load_KEY_NAME();
    if (!tapKey)
        throw ErrorMessage() << "invalid tap output key.";

    // Get the key's scan code to use as the keycode
    const ScanCode *scanCodes = tapKey->getScanCodes();
    if (tapKey->getScanCodesSize() == 0)
        throw ErrorMessage() << "tap output key has no scan codes.";

    uint16_t tapKeyCode = scanCodes[0].m_scan;

    // Check for duplicate assignment (warn and overwrite)
    if (m_setting->m_modTapActions.find(static_cast<uint8_t>(modNum)) != m_setting->m_modTapActions.end()) {
        if (m_log && m_soLog) {
            Acquire a(m_soLog);
            *m_log << m_currentFilename << " : warning: duplicate tap assignment for "
                   << modName << ", overwriting previous value." << std::endl;
        }
    }

    // Store the tap action
    m_setting->m_modTapActions[static_cast<uint8_t>(modNum)] = tapKeyCode;
}


// <MODIFIER_ASSIGNMENT>
void SettingLoader::load_MODIFIER_ASSIGNMENT()
{
    // <MODIFIER_NAME>
    Token *t = getToken();
    Modifier::Type mt;

    while (true) {
        Keymap::AssignMode am = Keymap::AM_notModifier;
        if      (*t == "!"  ) am = Keymap::AM_true, t = getToken();
        else if (*t == "!!" ) am = Keymap::AM_oneShot, t = getToken();
        else if (*t == "!!!") am = Keymap::AM_oneShotRepeatable, t = getToken();

        if      (*t == "shift") mt = Modifier::Type_Shift;
        else if (*t == "alt"  ||
                 *t == "meta" ||
                 *t == "menu" ) mt = Modifier::Type_Alt;
        else if (*t == "control" ||
                 *t == "ctrl" ) mt = Modifier::Type_Control;
        else if (*t == "windows" ||
                 *t == "win"  ) mt = Modifier::Type_Windows;
        else throw ErrorMessage() << "`" << *t
            << "': invalid modifier name.";

        if (am == Keymap::AM_notModifier)
            break;

        m_currentKeymap->addModifier(mt, Keymap::AO_overwrite, am, nullptr);
        if (isEOL())
            return;
        t = getToken();
    }

    // <ASSIGN_OP>
    t = getToken();
    Keymap::AssignOperator ao;
    if      (*t == "=" ) ao = Keymap::AO_new;
    else if (*t == "+=") ao = Keymap::AO_add;
    else if (*t == "-=") ao = Keymap::AO_sub;
    else  throw ErrorMessage() << "`" << *t << "': is unknown operator.";

    // <ASSIGN_MODE>? <KEY_NAME>
    while (!isEOL()) {
        // <ASSIGN_MODE>?
        t = getToken();
        Keymap::AssignMode am = Keymap::AM_normal;
        if      (*t == "!"  ) am = Keymap::AM_true, t = getToken();
        else if (*t == "!!" ) am = Keymap::AM_oneShot, t = getToken();
        else if (*t == "!!!") am = Keymap::AM_oneShotRepeatable, t = getToken();

        // <KEY_NAME>
        Key *key = m_setting->m_keyboard.searchKey(t->getString());
        if (!key)
            throw ErrorMessage() << "`" << *t << "': invalid key name.";

        // we can ignore warning C4701
        m_currentKeymap->addModifier(mt, ao, am, key);
        if (ao == Keymap::AO_new)
            ao = Keymap::AO_add;
    }
}


// <KEYSEQ_DEFINITION>
void SettingLoader::load_KEYSEQ_DEFINITION()
{
    if (*getToken() != "$")
        throw ErrorMessage() << "there must be `$' after `keyseq'";
    Token *name = getToken();
    if (*getToken() != "=")
        throw ErrorMessage() << "there must be `=' after keyseq naem";
    load_KEY_SEQUENCE(to_UTF_8(name->getString()), false, Modifier::Type_ASSIGN);
}


// <DEFINE>
void SettingLoader::load_DEFINE()
{
    m_setting->m_symbols.insert(getToken()->getString());
}


// <IF>
void SettingLoader::load_IF()
{
    if (!getToken()->isOpenParen())
        throw ErrorMessage() << "there must be `(' after `if'.";
    Token *t = getToken(); // <SYMBOL> or !
    bool isNot = false;
    if (*t == "!") {
        isNot = true;
        t = getToken(); // <SYMBOL>
    }

    bool doesSymbolExist = (m_setting->m_symbols.find(t->getString())
                            != m_setting->m_symbols.end());
    bool doesRead = ((doesSymbolExist && !isNot) ||
                     (!doesSymbolExist && isNot));
    if (0 < m_canReadStack.size())
        doesRead = doesRead && m_canReadStack.back();

    if (!getToken()->isCloseParen())
        throw ErrorMessage() << "there must be `)'.";

    m_canReadStack.push_back(doesRead);
    if (!isEOL()) {
        size_t len = m_canReadStack.size();
        load_LINE();
        if (len < m_canReadStack.size()) {
            bool r = m_canReadStack.back();
            m_canReadStack.pop_back();
            m_canReadStack[len - 1] = r && doesRead;
        } else if (len == m_canReadStack.size())
            m_canReadStack.pop_back();
        else
            ; // `end' found
    }
}


// <ELSE> <ELSEIF>
void SettingLoader::load_ELSE(bool i_isElseIf, const std::string &i_token)
{
    bool doesRead = !load_ENDIF(i_token);
    if (0 < m_canReadStack.size())
        doesRead = doesRead && m_canReadStack.back();
    m_canReadStack.push_back(doesRead);
    if (!isEOL()) {
        size_t len = m_canReadStack.size();
        if (i_isElseIf)
            load_IF();
        else
            load_LINE();
        if (len < m_canReadStack.size()) {
            bool r = m_canReadStack.back();
            m_canReadStack.pop_back();
            m_canReadStack[len - 1] = doesRead && r;
        } else if (len == m_canReadStack.size())
            m_canReadStack.pop_back();
        else
            ; // `end' found
    }
}


// <ENDIF>
bool SettingLoader::load_ENDIF(const std::string &i_token)
{
    if (m_canReadStack.size() == 0)
        throw ErrorMessage() << "unbalanced `" << i_token << "'";
    bool r = m_canReadStack.back();
    m_canReadStack.pop_back();
    return r;
}


// <LINE>
void SettingLoader::load_LINE()
{
    Token *i_token = getToken();

    // <COND_SYMBOL>
    if      (*i_token == "if" ||
             *i_token == "and") load_IF();
    else if (*i_token == "else") load_ELSE(false, to_UTF_8(i_token->getString()));
    else if (*i_token == "elseif" ||
             *i_token == "elsif"  ||
             *i_token == "elif"   ||
             *i_token == "or") load_ELSE(true, to_UTF_8(i_token->getString()));
    else if (*i_token == "endif") load_ENDIF("endif");
    else if (0 < m_canReadStack.size() && !m_canReadStack.back()) {
        while (!isEOL())
            getToken();
    } else if (*i_token == "define") load_DEFINE();
    // <INCLUDE>
    else if (*i_token == "include") load_INCLUDE();
    // <KEYBOARD_DEFINITION>
    else if (*i_token == "def") load_KEYBOARD_DEFINITION();
    // <KEYMAP_DEFINITION>
    else if (*i_token == "keymap"  ||
             *i_token == "keymap2" ||
             *i_token == "window") load_KEYMAP_DEFINITION(i_token);
    // <KEY_ASSIGN>
    else if (*i_token == "key") load_KEY_ASSIGN();
    // <EVENT_ASSIGN>
    else if (*i_token == "event") load_EVENT_ASSIGN();
    // <MOD_ASSIGN> or <MODIFIER_ASSIGNMENT>
    else if (*i_token == "mod") {
        // Check if next token is "assign"
        if (!isEOL() && *lookToken() == "assign")
            load_MOD_ASSIGN();
        else
            load_MODIFIER_ASSIGNMENT();
    }
    // <KEYSEQ_DEFINITION>
    else if (*i_token == "keyseq") load_KEYSEQ_DEFINITION();
    else
        throw ErrorMessage() << "syntax error `" << *i_token << "'.";
}


// prefix sort predicate used in load(const string &)
static bool prefixSortPred(const std::string &i_a, const std::string &i_b)
{
    return i_b.size() < i_a.size();
}


/*
  _UNICODE: read file (UTF-16 LE/BE, UTF-8, locale specific multibyte encoding)
  _MBCS: read file
*/
static bool readFile(std::string *o_data, const std::string &i_filename)
{
#ifdef _WIN32
#ifdef _UNICODE
    std::wstring tFilename = yamy::platform::utf8_to_wstring(i_filename);
#else
    std::string tFilename = i_filename;
#endif

    // get size of file
#if 0
    // bcc's _wstat cannot obtain file size
    struct _stat sbuf;
    if (_tstat(tFilename.c_str(), &sbuf) < 0 || sbuf.st_size == 0)
        return false;
#else
    // so, we use _wstati64 for bcc
    struct stati64_t sbuf;
    if (_tstati64(tFilename.c_str(), &sbuf) < 0 || sbuf.st_size == 0)
        return false;
    // following check is needed to cast sbuf.st_size to size_t safely
    // this cast occurs because of above workaround for bcc
    if (sbuf.st_size > UINT_MAX)
        return false;
#endif

    // open
    FILE *fp = _tfopen(tFilename.c_str(), _T("rb"));
#else // Linux
    const std::string &tFilename = i_filename;
    struct stat sbuf;
    if (stat(tFilename.c_str(), &sbuf) < 0 || sbuf.st_size == 0)
        return false;
    FILE *fp = fopen(tFilename.c_str(), "rb");
#endif
    if (!fp)
        return false;

    // read file
    Array<BYTE> buf(static_cast<size_t>(sbuf.st_size) + 1);
    if (fread(buf.get(), static_cast<size_t>(sbuf.st_size), 1, fp) != 1) {
        fclose(fp);
        return false;
    }
    buf.get()[sbuf.st_size] = 0;            // mbstowcs() requires nullptr
    // terminated string

#ifdef _UNICODE
    //
    if (buf.get()[0] == 0xffU && buf.get()[1] == 0xfeU &&
            sbuf.st_size % 2 == 0)
        // UTF-16 Little Endien
    {
        size_t size = static_cast<size_t>(sbuf.st_size) / 2;
        std::wstring wdata;
        wdata.resize(size);
        BYTE *p = buf.get();
        for (size_t i = 0; i < size; ++ i) {
            wchar_t c = static_cast<wchar_t>(*p ++);
            c |= static_cast<wchar_t>(*p ++) << 8;
            wdata[i] = c;
        }
        *o_data = yamy::platform::wstring_to_utf8(wdata);
        fclose(fp);
        return true;
    }

    //
    if (buf.get()[0] == 0xfeU && buf.get()[1] == 0xffU &&
            sbuf.st_size % 2 == 0)
        // UTF-16 Big Endien
    {
        size_t size = static_cast<size_t>(sbuf.st_size) / 2;
        std::wstring wdata;
        wdata.resize(size);
        BYTE *p = buf.get();
        for (size_t i = 0; i < size; ++ i) {
            wchar_t c = static_cast<wchar_t>(*p ++) << 8;
            c |= static_cast<wchar_t>(*p ++);
            wdata[i] = c;
        }
        *o_data = yamy::platform::wstring_to_utf8(wdata);
        fclose(fp);
        return true;
    }

    // try UTF-8
    {
        Array<wchar_t> wbuf(static_cast<size_t>(sbuf.st_size));
        BYTE *f = buf.get();
        BYTE *end = buf.get() + sbuf.st_size;
        wchar_t *d = wbuf.get();
        enum { STATE_1, STATE_2of2, STATE_2of3, STATE_3of3 } state = STATE_1;

        if (f + 3 <= end && f[0] == 0xef && f[1] == 0xbb && f[2] == 0xbf)
            f += 3;

        while (f != end) {
            switch (state) {
            case STATE_1:
                if (!(*f & 0x80))            // 0xxxxxxx: 00-7F
                    *d++ = static_cast<wchar_t>(*f++);
                else if ((*f & 0xe0) == 0xc0) {    // 110xxxxx 10xxxxxx: 0080-07FF
                    *d = ((static_cast<wchar_t>(*f++) & 0x1f) << 6);
                    state = STATE_2of2;
                } else if ((*f & 0xf0) == 0xe0)        // 1110xxxx 10xxxxxx 10xxxxxx:
                    // 0800 - FFFF
                {
                    *d = ((static_cast<wchar_t>(*f++) & 0x0f) << 12);
                    state = STATE_2of3;
                } else
                    goto not_UTF_8;
                break;

            case STATE_2of2:
            case STATE_3of3:
                if ((*f & 0xc0) != 0x80)
                    goto not_UTF_8;
                *d++ |= (static_cast<wchar_t>(*f++) & 0x3f);
                state = STATE_1;
                break;

            case STATE_2of3:
                if ((*f & 0xc0) != 0x80)
                    goto not_UTF_8;
                *d |= ((static_cast<wchar_t>(*f++) & 0x3f) << 6);
                state = STATE_3of3;
                break;
            }
        }
        {
            std::wstring wdata(wbuf.get(), d);
            *o_data = yamy::platform::wstring_to_utf8(wdata);
            fclose(fp);
            return true;
        }

not_UTF_8:
        ;
    }

    // try multibyte charset
    size_t wsize = mbstowcs(nullptr, reinterpret_cast<char *>(buf.get()), 0);
    if (wsize != size_t(-1)) {
        Array<wchar_t> wbuf(wsize);
        mbstowcs(wbuf.get(), reinterpret_cast<char *>(buf.get()), wsize);
        std::wstring wdata(wbuf.get(), wbuf.get() + wsize);
        *o_data = yamy::platform::wstring_to_utf8(wdata);
        fclose(fp);
        return true;
    }
#endif // _UNICODE

    // assume ascii
    o_data->resize(static_cast<size_t>(sbuf.st_size));
    for (off_t i = 0; i < sbuf.st_size; ++ i)
        (*o_data)[i] = buf.get()[i];
    fclose(fp);
    return true;
}


// load (called from load(Setting *, const std::string &) only)
void SettingLoader::load(const std::string &i_filename)
{
    m_currentFilename = i_filename;

    std::string data;
    if (!readFile(&data, m_currentFilename)) {
        Acquire a(m_soLog);
        *m_log << m_currentFilename << " : error: file not found" << std::endl;
#if 1
        *m_log << data << std::endl;
#endif
        m_isThereAnyError = true;
        return;
    }

    loadFromData(data);
}


// load setting from data string
void SettingLoader::loadFromData(const std::string &data)
{
    // prefix
    if (m_prefixesRefCcount == 0) {
        static const char *prefixes[] = {
            "=", "=>", "&&", "||", ":", "$", "&",
            "-=", "+=", "!!!", "!!", "!",
            "E0-", "E1-",            // <SCAN_CODE_EXTENTION>
            "S-", "A-", "M-", "C-",    // <BASIC_MODIFIER>
            "W-", "*", "~",
            "U-", "D-",            // <KEYSEQ_MODIFIER>
            "R-", "IL-", "IC-", "I-",    // <ASSIGN_MODIFIER>
            "NL-", "CL-", "SL-", "KL-",
            "MAX-", "MIN-", "MMAX-", "MMIN-",
            "T-", "TS-",
            "L0-", "L1-", "L2-", "L3-", "L4-",
            "L5-", "L6-", "L7-", "L8-", "L9-",
        };
        m_prefixes = new std::vector<std::string>;
        for (size_t i = 0; i < NUMBER_OF(prefixes); ++ i)
            m_prefixes->push_back(prefixes[i]);
        std::sort(m_prefixes->begin(), m_prefixes->end(), prefixSortPred);
    }
    m_prefixesRefCcount ++;

    // create parser
    Parser parser(data.c_str(), data.size());
    parser.setPrefixes(m_prefixes);

    while (true) {
        try {
            if (!parser.getLine(&m_tokens))
                break;
            m_ti = m_tokens.begin();
        } catch (ErrorMessage &e) {
            if (m_log && m_soLog) {
                Acquire a(m_soLog);
                *m_log << m_currentFilename << "(" << parser.getLineNumber()
                << ") : error: " << e << std::endl;
            }
            m_isThereAnyError = true;
            continue;
        }

        try {
            load_LINE();
            if (!isEOL())
                throw WarningMessage() << "back garbage is ignored.";
        } catch (WarningMessage &w) {
            if (m_log && m_soLog) {
                Acquire a(m_soLog);
                *m_log << m_currentFilename << "(" << parser.getLineNumber()
                << ") : warning: " << w << std::endl;
            }
        } catch (ErrorMessage &e) {
            if (m_log && m_soLog) {
                Acquire a(m_soLog);
                *m_log << m_currentFilename << "(" << parser.getLineNumber()
                << ") : error: " << e << std::endl;
            }
            m_isThereAnyError = true;
        }
    }

    // m_prefixes
    -- m_prefixesRefCcount;
    if (m_prefixesRefCcount == 0)
        delete m_prefixes;

    if (0 < m_canReadStack.size()) {
        Acquire a(m_soLog);
        *m_log << m_currentFilename << "(" << parser.getLineNumber()
        << ") : error: unbalanced `if'.  "
        << "you forget `endif', didn'i_token you?"
        << std::endl;
        m_isThereAnyError = true;
    }
}


// is the filename readable ?
bool SettingLoader::isReadable(const std::string &i_filename,
                               int i_debugLevel) const
{
    if (i_filename.empty())
        return false;

#ifdef _UNICODE
    std::wstring tFilename = yamy::platform::utf8_to_wstring(i_filename);
#else
    std::string tFilename = i_filename;
#endif
    tifstream ist(tFilename.c_str());
    if (ist.good()) {
        if (m_log && m_soLog) {
            Acquire a(m_soLog, 0);
            *m_log << "  loading: " << i_filename << std::endl;
        }
        return true;
    } else {
        if (m_log && m_soLog) {
            Acquire a(m_soLog, i_debugLevel);
            *m_log << "not found: " << i_filename << std::endl;
        }
        return false;
    }
}


// get filename
// get filename
bool SettingLoader::getFilename(const std::string &i_name, std::string *o_path,
                                int i_debugLevel) const
{
    // the default filename is ".mayu"
    const std::string &name = i_name.empty() ? ".mayu" : i_name;

    bool isFirstTime = true;

    while (true) {
        // find file from registry
        if (i_name.empty()) {            // called not from 'include'
            Setting::Symbols symbols;
            std::string sPath;
            if (m_config && getFilenameFromConfig(*m_config, nullptr, &sPath, &symbols)) {
                *o_path = sPath;
                if (o_path->empty())
                    // find file from home directory
                {
                    HomeDirectories pathes;
                    getHomeDirectories(m_config, &pathes);
                    for (HomeDirectories::iterator
                            i = pathes.begin(); i != pathes.end(); ++ i) {
                        *o_path = to_UTF_8(*i) + "\\" + name;
                        if (isReadable(*o_path, i_debugLevel))
                            goto add_symbols;
                    }
                    return false;
                } else {
                    if (!isReadable(*o_path, i_debugLevel))
                        return false;
                }
add_symbols:
                for (Setting::Symbols::iterator
                        i = symbols.begin(); i != symbols.end(); ++ i)
                    m_setting->m_symbols.insert(*i);
                return true;
            }
        }

        if (!isFirstTime)
            return false;

        // find file from home directory
        HomeDirectories pathes;

        // check relative to current file
        if (!m_currentFilename.empty()) {
            // Use filesystem::path to extract directory
            std::filesystem::path filePath(m_currentFilename);
            std::string dir = filePath.parent_path().string();
            if (!dir.empty())
                pathes.push_back(dir);
        }

        getHomeDirectories(m_config, &pathes);
        for (HomeDirectories::iterator i = pathes.begin(); i != pathes.end(); ++ i) {
            // Use filesystem::path to handle path separators automatically
            std::filesystem::path fullPath = std::filesystem::path(to_UTF_8(*i)) / name;
            *o_path = fullPath.string();
            if (isReadable(*o_path, i_debugLevel))
                return true;
        }

        if (!i_name.empty())
            return false;                // called by 'include'

#if defined(_WIN32) && !defined(BUILD_QT_GUI)
        if (!DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_setting),
                       nullptr, (DLGPROC)dlgSetting_dlgProc))
            return false;
#else
        // On Linux, no dialog - just return false
        return false;
#endif
    }
}


// constructor for root loader (creates own IncludeContext)
SettingLoader::SettingLoader(SyncObject *i_soLog, std::ostream *i_log, const ConfigStore *i_config)
        : m_setting(nullptr),
        m_ast(std::make_shared<yamy::ast::ConfigAST>()),
        m_config(i_config),
        m_isThereAnyError(false),
        m_soLog(i_soLog),
        m_log(i_log),
        m_includeContext(new yamy::IncludeContext()),  // Create owned context
        m_ownsIncludeContext(true),                    // We own it
        m_currentKeymap(nullptr)
{
    m_defaultKeySeqModifier =
        m_defaultAssignModifier.release(Modifier::Type_ImeComp);
}

// constructor for child loader (shares IncludeContext)
SettingLoader::SettingLoader(SyncObject *i_soLog, std::ostream *i_log, const ConfigStore *i_config, yamy::IncludeContext& i_includeContext, std::shared_ptr<yamy::ast::ConfigAST> ast)
        : m_setting(nullptr),
        m_ast(ast),
        m_config(i_config),
        m_isThereAnyError(false),
        m_soLog(i_soLog),
        m_log(i_log),
        m_includeContext(&i_includeContext),  // Use shared context (not owned)
        m_ownsIncludeContext(false),          // We don't own it
        m_currentKeymap(nullptr)
{
    m_defaultKeySeqModifier =
        m_defaultAssignModifier.release(Modifier::Type_ImeComp);
}

// destructor
SettingLoader::~SettingLoader()
{
    // Only delete context if we own it (root loader)
    if (m_ownsIncludeContext && m_includeContext) {
        delete m_includeContext;
    }
}


// initialize
bool SettingLoader::initialize(Setting *i_setting)
{
    m_setting = i_setting;
    m_isThereAnyError = false;

    // create global keymap's default keySeq
    FunctionData *fd = createFunctionData("OtherWindowClass");
    ActionFunction af(fd);
    KeySeq *globalDefault = m_setting->m_keySeqs.add(KeySeq("").add(af));

    // add default keymap
    m_currentKeymap = m_setting->m_keymaps.add(
                          Keymap(Keymap::Type_windowOr, "Global", "", "",
                                 globalDefault, nullptr));
    return true;
}


/* load m_setting
   If called by "include", 'filename' describes filename.
   Otherwise the 'filename' is empty.
 */
bool SettingLoader::load(Setting *i_setting, const std::string &i_filename)
{
    initialize(i_setting);

    std::string path;
    if (!getFilename(i_filename, &path)) {
        if (i_filename.empty()) {
            Acquire a(m_soLog);
            getFilename(i_filename, &path, 0);    // show filenames
            return false;
        } else
            throw ErrorMessage() << "`" << i_filename
            << "': no such file or other error.";
    }

    // load
    load(path);

    // finalize
    if (i_filename.empty())
        m_setting->m_keymaps.adjustModifier(m_setting->m_keyboard);

    return !m_isThereAnyError;
}


std::vector<std::string> *SettingLoader::m_prefixes; // m_prefixes terminal symbol
size_t SettingLoader::m_prefixesRefCcount;    /* reference count of
                           m_prefixes */
