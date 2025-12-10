// setting_loader.cpp


#include "misc.h"

#ifdef _WIN32
#include "dlgsetting.h"
#endif
#include "errormessage.h"
#include "mayu.h"
#ifdef _WIN32
#include "mayurc.h"
#endif
#include "setting.h"
#include "setting_loader.h"
#include "windowstool.h"
#include "vkeytable.h"
#include "array.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sys/stat.h>
#include <climits>

#ifndef _WIN32
#include <unistd.h>
#endif

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
    if (i_doesThrow) {
        ErrorMessage e;
        e << "there must be `(' after `&";
        if (i_name) e << i_name;
        e << "'.";
        throw e;
    }
    return false;
}

#ifdef _UNICODE
bool SettingLoader::getOpenParen(bool i_doesThrow, const wchar_t *i_name) {
    return getOpenParen(i_doesThrow, i_name ? yamy::platform::wstring_to_utf8(i_name).c_str() : nullptr);
}
#endif


// argument ")"
bool SettingLoader::getCloseParen(bool i_doesThrow, const char *i_name)
{
    if (!isEOL() && lookToken()->isCloseParen()) {
        getToken();
        return true;
    }
    if (i_doesThrow) {
        ErrorMessage e;
        e << "`&";
        if (i_name) e << i_name;
        e << "': too many arguments.";
        throw e;
    }
    return false;
}

#ifdef _UNICODE
bool SettingLoader::getCloseParen(bool i_doesThrow, const wchar_t *i_name) {
    return getCloseParen(i_doesThrow, i_name ? yamy::platform::wstring_to_utf8(i_name).c_str() : nullptr);
}
#endif


// argument ","
bool SettingLoader::getComma(bool i_doesThrow, const char *i_name)
{
    if (!isEOL() && lookToken()->isComma()) {
        getToken();
        return true;
    }
    if (i_doesThrow) {
        ErrorMessage e;
        e << "`&";
        if (i_name) e << i_name;
        e << "': comma expected.";
        throw e;
    }
    return false;
}

#ifdef _UNICODE
bool SettingLoader::getComma(bool i_doesThrow, const wchar_t *i_name) {
    return getComma(i_doesThrow, i_name ? yamy::platform::wstring_to_utf8(i_name).c_str() : nullptr);
}
#endif


// <INCLUDE>
void SettingLoader::load_INCLUDE()
{
    SettingLoader loader(m_soLog, m_log, m_config);
    loader.m_currentFilename = m_currentFilename;
    loader.m_defaultAssignModifier = m_defaultAssignModifier;
    loader.m_defaultKeySeqModifier = m_defaultKeySeqModifier;
    if (!loader.load(m_setting, (*getToken()).getString()))
        m_isThereAnyError = true;
}


// <SCAN_CODES>
void SettingLoader::load_SCAN_CODES(Key *o_key)
{
    for (int j = 0; j < Key::MAX_SCAN_CODES_SIZE && !isEOL(); ++ j) {
        ScanCode sc;
        sc.m_flags = 0;
        while (true) {
            Token *t = getToken();
            if (t->isNumber()) {
                sc.m_scan = (u_char)t->getNumber();
                o_key->addScanCode(sc);
                break;
            }
            if      (*t == "E0-") sc.m_flags |= ScanCode::E0;
            else if (*t == "E1-") sc.m_flags |= ScanCode::E1;
            else  throw ErrorMessage() << "`" << *t
                << "': invalid modifier.";
        }
    }
}


// <DEFINE_KEY>
void SettingLoader::load_DEFINE_KEY()
{
    Token *t = getToken();
    Key key;

    // <KEY_NAMES>
    if (*t == '(') {
        key.addName(getToken()->getString());
        while (t = getToken(), *t != ')')
            key.addName(t->getString());
        if (*getToken() != "=")
            throw ErrorMessage() << "there must be `=' after `)'.";
    } else {
        key.addName(t->getString());
        while (t = getToken(), *t != "=")
            key.addName(t->getString());
    }

    load_SCAN_CODES(&key);
    m_setting->m_keyboard.addKey(key);
}


// <DEFINE_MODIFIER>
void SettingLoader::load_DEFINE_MODIFIER()
{
    Token *t = getToken();
    Modifier::Type mt;
    if      (*t == "shift"  ) mt = Modifier::Type_Shift;
    else if (*t == "alt"     ||
             *t == "meta"    ||
             *t == "menu"   ) mt = Modifier::Type_Alt;
    else if (*t == "control" ||
             *t == "ctrl"   ) mt = Modifier::Type_Control;
    else if (*t == "windows" ||
             *t == "win"    ) mt = Modifier::Type_Windows;
    else throw ErrorMessage() << "`" << *t
        << "': invalid modifier name.";

    if (*getToken() != "=")
        throw ErrorMessage() << "there must be `=' after modifier name.";

    while (!isEOL()) {
        t = getToken();
        Key *key =
            m_setting->m_keyboard.searchKeyByNonAliasName(t->getString());
        if (!key)
            throw ErrorMessage() << "`" << *t << "': invalid key name.";
        m_setting->m_keyboard.addModifier(mt, key);
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

    load_SCAN_CODES(key);
}


// <DEFINE_ALIAS>
void SettingLoader::load_DEFINE_ALIAS()
{
    Token *name = getToken();

    if (*getToken() != "=")
        throw ErrorMessage() << "there must be `=' after `alias'.";

    Token *t = getToken();
    Key *key = m_setting->m_keyboard.searchKeyByNonAliasName(t->getString());
    if (!key)
        throw ErrorMessage() << "`" << *t << "': invalid key name.";
    m_setting->m_keyboard.addAlias(name->getString(), key);
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

    // <DEFINE_OPTION>
    else if (*t == "option") load_DEFINE_OPTION();

    //
    else throw ErrorMessage() << "syntax error `" << *t << "'.";
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
            { "M0-", Modifier::Type_Mod0 },
            { "M1-", Modifier::Type_Mod1 },
            { "M2-", Modifier::Type_Mod2 },
            { "M3-", Modifier::Type_Mod3 },
            { "M4-", Modifier::Type_Mod4 },
            { "M5-", Modifier::Type_Mod5 },
            { "M6-", Modifier::Type_Mod6 },
            { "M7-", Modifier::Type_Mod7 },
            { "M8-", Modifier::Type_Mod8 },
            { "M9-", Modifier::Type_Mod9 },
            { "M10-", Modifier::Type_Mod10 },
            { "M11-", Modifier::Type_Mod11 },
            { "M12-", Modifier::Type_Mod12 },
            { "M13-", Modifier::Type_Mod13 },
            { "M14-", Modifier::Type_Mod14 },
            { "M15-", Modifier::Type_Mod15 },
            { "M16-", Modifier::Type_Mod16 },
            { "M17-", Modifier::Type_Mod17 },
            { "M18-", Modifier::Type_Mod18 },
            { "M19-", Modifier::Type_Mod19 },
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


// <KEY_NAME>
Key *SettingLoader::load_KEY_NAME()
{
    Token *t = getToken();
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
                          Keymap(type, name->getString(), windowClassName, windowTitleName,
                                 nullptr, nullptr));

    if (doesLoadDefaultKeySeq) {
        Token *t = lookToken();
        // <KEYMAP_PARENT>
        if (*t == ":") {
            getToken();
            t = getToken();
            parentKeymap = m_setting->m_keymaps.searchByName(t->getString());
            if (!parentKeymap)
                throw ErrorMessage() << "`" << *t
                << "': unknown keymap name.";
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
                     KeySeq(name->getString()).add(ActionFunction(fd)));
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
void SettingLoader::load_ARGUMENT(unsigned __int64 *o_arg)
{
    *o_arg = getToken()->getNumber();
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(__int64 *o_arg)
{
    *o_arg = getToken()->getNumber();
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(std::string *o_arg)
{
    *o_arg = getToken()->getString();
}


// <ARGUMENT>
void SettingLoader::load_ARGUMENT(std::list<std::string> *o_arg)
{
    while (true) {
        if (!lookToken()->isString())
            return;
        o_arg->push_back(getToken()->getString());

        if (!lookToken()->isComma())
            return;
        getToken();
    }
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(Regex *o_arg)
{
    *o_arg = Regex(getToken()->getRegexp());
}


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
    keymap = m_setting->m_keymaps.searchByName(t->getString());
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
        keySeq = m_setting->m_keySeqs.searchByName(t->getString());
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
    *o_arg = StrExprArg(t->getString(), type);
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
            KeySeq *ks = m_setting->m_keySeqs.searchByName(t->getString());
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
        else if (*t == "mod0" ) mt = Modifier::Type_Mod0;
        else if (*t == "mod1" ) mt = Modifier::Type_Mod1;
        else if (*t == "mod2" ) mt = Modifier::Type_Mod2;
        else if (*t == "mod3" ) mt = Modifier::Type_Mod3;
        else if (*t == "mod4" ) mt = Modifier::Type_Mod4;
        else if (*t == "mod5" ) mt = Modifier::Type_Mod5;
        else if (*t == "mod6" ) mt = Modifier::Type_Mod6;
        else if (*t == "mod7" ) mt = Modifier::Type_Mod7;
        else if (*t == "mod8" ) mt = Modifier::Type_Mod8;
        else if (*t == "mod9" ) mt = Modifier::Type_Mod9;
        else if (*t == "mod10" ) mt = Modifier::Type_Mod10;
        else if (*t == "mod11" ) mt = Modifier::Type_Mod11;
        else if (*t == "mod12" ) mt = Modifier::Type_Mod12;
        else if (*t == "mod13" ) mt = Modifier::Type_Mod13;
        else if (*t == "mod14" ) mt = Modifier::Type_Mod14;
        else if (*t == "mod15" ) mt = Modifier::Type_Mod15;
        else if (*t == "mod16" ) mt = Modifier::Type_Mod16;
        else if (*t == "mod17" ) mt = Modifier::Type_Mod17;
        else if (*t == "mod18" ) mt = Modifier::Type_Mod18;
        else if (*t == "mod19" ) mt = Modifier::Type_Mod19;
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
    load_KEY_SEQUENCE(name->getString(), false, Modifier::Type_ASSIGN);
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
    else if (*i_token == "else") load_ELSE(false, i_token->getString());
    else if (*i_token == "elseif" ||
             *i_token == "elsif"  ||
             *i_token == "elif"   ||
             *i_token == "or") load_ELSE(true, i_token->getString());
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
    // <MODIFIER_ASSIGNMENT>
    else if (*i_token == "mod") load_MODIFIER_ASSIGNMENT();
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
  read file (UTF-16 LE/BE, UTF-8, locale specific multibyte encoding)
  Output is UTF-8 std::string.
*/
static bool readFile(std::string *o_data, const std::string &i_filename)
{
    // get size of file
#ifdef _WIN32
    struct _stati64 sbuf;
    std::wstring wfilename = yamy::platform::utf8_to_wstring(i_filename);
    if (_wstati64(wfilename.c_str(), &sbuf) < 0 || sbuf.st_size == 0)
        return false;
#else
    struct stat sbuf;
    if (stat(i_filename.c_str(), &sbuf) < 0 || sbuf.st_size == 0)
        return false;
#endif

    // check size limit
    if (sbuf.st_size > UINT_MAX)
        return false;

    // open
#ifdef _WIN32
    FILE *fp = _wfopen(wfilename.c_str(), L"rb");
#else
    FILE *fp = fopen(i_filename.c_str(), "rb");
#endif
    if (!fp)
        return false;

    // read file
    Array<BYTE> buf(static_cast<size_t>(sbuf.st_size) + 1);
    if (fread(buf.get(), static_cast<size_t>(sbuf.st_size), 1, fp) != 1) {
        fclose(fp);
        return false;
    }
    buf.get()[sbuf.st_size] = 0;

    // Detect encoding and convert to UTF-8

    // Check BOM for UTF-16 LE
    if (sbuf.st_size >= 2 && buf.get()[0] == 0xffU && buf.get()[1] == 0xfeU &&
            sbuf.st_size % 2 == 0)
    {
        size_t size = static_cast<size_t>(sbuf.st_size) / 2;
        std::wstring wstr;
        wstr.resize(size);
        BYTE *p = buf.get();
        for (size_t i = 0; i < size; ++ i) {
            wchar_t c = static_cast<wchar_t>(*p ++);
            c |= static_cast<wchar_t>(*p ++) << 8;
            wstr[i] = c;
        }
        fclose(fp);
        *o_data = yamy::platform::wstring_to_utf8(wstr);
        return true;
    }

    // Check BOM for UTF-16 BE
    if (sbuf.st_size >= 2 && buf.get()[0] == 0xfeU && buf.get()[1] == 0xffU &&
            sbuf.st_size % 2 == 0)
    {
        size_t size = static_cast<size_t>(sbuf.st_size) / 2;
        std::wstring wstr;
        wstr.resize(size);
        BYTE *p = buf.get();
        for (size_t i = 0; i < size; ++ i) {
            wchar_t c = static_cast<wchar_t>(*p ++) << 8;
            c |= static_cast<wchar_t>(*p ++);
            wstr[i] = c;
        }
        fclose(fp);
        *o_data = yamy::platform::wstring_to_utf8(wstr);
        return true;
    }

    // Check BOM for UTF-8 (EF BB BF)
    bool hasUtf8Bom = (sbuf.st_size >= 3 && buf.get()[0] == 0xef && buf.get()[1] == 0xbb && buf.get()[2] == 0xbf);
    if (hasUtf8Bom) {
        *o_data = std::string(reinterpret_cast<char*>(buf.get() + 3), sbuf.st_size - 3);
        fclose(fp);
        return true;
    }

    // Try UTF-8 detection logic (simple)
    bool validUtf8 = true;
    const BYTE *p = buf.get();
    const BYTE *end = buf.get() + sbuf.st_size;
    while (p < end) {
        if (!(*p & 0x80)) { p++; continue; }
        int len = 0;
        if ((*p & 0xe0) == 0xc0) len = 1;
        else if ((*p & 0xf0) == 0xe0) len = 2;
        else if ((*p & 0xf8) == 0xf0) len = 3; // 4-byte UTF-8
        else { validUtf8 = false; break; }

        p++;
        if (p + len > end) { validUtf8 = false; break; }
        for (int i=0; i<len; ++i) {
            if ((*p & 0xc0) != 0x80) { validUtf8 = false; break; }
            p++;
        }
        if (!validUtf8) break;
    }

    if (validUtf8) {
        *o_data = std::string(reinterpret_cast<char*>(buf.get()), sbuf.st_size);
        fclose(fp);
        return true;
    }

#ifdef _WIN32
    // Fallback: assume MBCS (CP_ACP) and convert to UTF-8
    size_t wsize = MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<char*>(buf.get()), sbuf.st_size, nullptr, 0);
    if (wsize > 0) {
        std::vector<wchar_t> wbuf(wsize + 1);
        MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<char*>(buf.get()), sbuf.st_size, wbuf.data(), wsize);
        wbuf[wsize] = 0;
        *o_data = yamy::platform::wstring_to_utf8(wbuf.data());
        fclose(fp);
        return true;
    }
#else
    *o_data = std::string(reinterpret_cast<char*>(buf.get()), sbuf.st_size);
#endif

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
        *m_log << to_tstring(m_currentFilename) << " : error: file not found" << std::endl;
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
            "M0-", "M1-", "M2-", "M3-", "M4-",
            "M5-", "M6-", "M7-", "M8-", "M9-",
            "M10-", "M11-", "M12-", "M13-", "M14-",
            "M15-", "M16-", "M17-", "M18-", "M19-",
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
                *m_log << to_tstring(m_currentFilename) << "(" << parser.getLineNumber()
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
                *m_log << to_tstring(m_currentFilename) << "(" << parser.getLineNumber()
                << ") : warning: " << w << std::endl;
            }
        } catch (ErrorMessage &e) {
            if (m_log && m_soLog) {
                Acquire a(m_soLog);
                *m_log << to_tstring(m_currentFilename) << "(" << parser.getLineNumber()
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
        *m_log << to_tstring(m_currentFilename) << "(" << parser.getLineNumber()
        << ") : error: unbalanced `if'.  "
        << "you forget `endif', didn't you?"
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

#ifdef _WIN32
    // Use wide stream/path on Windows
    std::wstring wname = yamy::platform::utf8_to_wstring(i_filename);
    std::ifstream ist(wname.c_str());
#else
    std::ifstream ist(i_filename.c_str());
#endif

    if (ist.good()) {
        if (m_log && m_soLog) {
            Acquire a(m_soLog, 0);
            *m_log << "  loading: " << to_tstring(i_filename) << std::endl;
        }
        return true;
    } else {
        if (m_log && m_soLog) {
            Acquire a(m_soLog, i_debugLevel);
            *m_log << "not found: " << to_tstring(i_filename) << std::endl;
        }
        return false;
    }
}


// get filename
bool SettingLoader::getFilename(const std::string &i_name, std::string *o_path,
                                int i_debugLevel) const
{
    // the default filename is ".mayu"
    const std::string &name = i_name.empty() ? std::string(".mayu") : i_name;

    bool isFirstTime = true;

    while (true) {
        // find file from registry
        if (i_name.empty()) {            // called not from 'include'
            Setting::Symbols symbols;
            if (m_config && getFilenameFromConfig(*m_config, nullptr, o_path, &symbols)) {
                if (o_path->empty())
                    // find file from home directory
                {
                    HomeDirectories pathes;
                    getHomeDirectories(m_config, &pathes);
                    for (HomeDirectories::iterator
                            i = pathes.begin(); i != pathes.end(); ++ i) {
                        *o_path = *i + "\\" + name;
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
            std::string dir = pathRemoveFileSpec(m_currentFilename);
            if (!dir.empty())
                pathes.push_back(dir);
        }

        getHomeDirectories(m_config, &pathes);
        for (HomeDirectories::iterator i = pathes.begin(); i != pathes.end(); ++ i) {
            *o_path = *i + "\\" + name;
            if (isReadable(*o_path, i_debugLevel))
                return true;
        }

        if (!i_name.empty())
            return false;                // called by 'include'

#ifdef _WIN32
        if (!DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_setting),
                       nullptr, (DLGPROC)dlgSetting_dlgProc))
            return false;
#else
        return false;
#endif
    }
}


// constructor
SettingLoader::SettingLoader(SyncObject *i_soLog, tostream *i_log, const ConfigStore *i_config)
        : m_setting(nullptr),
        m_config(i_config),
        m_isThereAnyError(false),
        m_soLog(i_soLog),
        m_log(i_log),
        m_currentKeymap(nullptr)
{
    m_defaultKeySeqModifier =
        m_defaultAssignModifier.release(Modifier::Type_ImeComp);
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
            throw ErrorMessage() << "`" << to_tstring(i_filename)
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
