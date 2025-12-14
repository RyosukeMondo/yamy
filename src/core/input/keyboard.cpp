//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// setting.cpp


#include "keyboard.h"

#include <algorithm>
#include <gsl/gsl>


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Key


// add a name or an alias of key
void Key::addName(const std::string &i_name)
{
    m_names.push_back(i_name);
}


// add a scan code
void Key::addScanCode(const ScanCode &i_sc)
{
    m_scanCodes.push_back(i_sc);
}


// initializer
Key &Key::initialize()
{
    m_names.clear();
    m_isPressed = false;
    m_isPressedOnWin32 = false;
    m_isPressedByAssign = false;
    m_scanCodes.clear();
    return *this;
}


// equation by name (UTF-8-aware case-insensitive comparison)
bool Key::operator==(const std::string &i_name) const
{
    return std::find_if(m_names.begin(), m_names.end(),
        [&i_name](const std::string &name) {
            return strcasecmp_utf8(name.c_str(), i_name.c_str()) == 0;
        }) != m_names.end();
}


// is the scan code of this key ?
bool Key::isSameScanCode(const Key &i_key) const
{
    if (m_scanCodes.size() != i_key.m_scanCodes.size())
        return false;
    return isPrefixScanCode(i_key);
}


// is the key's scan code the prefix of this key's scan code ?
bool Key::isPrefixScanCode(const Key &i_key) const
{
    for (size_t i = 0; i < i_key.m_scanCodes.size(); ++ i)
        if (m_scanCodes[i] != i_key.m_scanCodes[i])
            return false;
    return true;
}


// stream output
std::ostream &operator<<(std::ostream &i_ost, const Key &i_mk)
{
    return i_ost << i_mk.getName();
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Modifier


Modifier::Modifier()
        : m_modifiers(0),
        m_dontcares(0)
{
    ASSERT(Type_end <= (sizeof(MODIFIERS) * 8));
    static const Type defaultDontCare[] = {
        Type_Up, Type_Down, Type_Repeat,
        Type_ImeLock, Type_ImeComp, Type_NumLock, Type_CapsLock, Type_ScrollLock,
        Type_KanaLock,
        Type_Maximized, Type_Minimized, Type_MdiMaximized, Type_MdiMinimized,
        Type_Touchpad, Type_TouchpadSticky,
        Type_Lock0, Type_Lock1, Type_Lock2, Type_Lock3, Type_Lock4,
        Type_Lock5, Type_Lock6, Type_Lock7, Type_Lock8, Type_Lock9,
    };
    for (size_t i = 0; i < NUMBER_OF(defaultDontCare); ++ i)
        dontcare(defaultDontCare[i]);
}


// add m's modifiers where this dontcare
void Modifier::add(const Modifier &i_m)
{
    for (int i = 0; i < Type_end; ++ i) {
        if (isDontcare(static_cast<Modifier::Type>(i))) {
            if (!i_m.isDontcare(static_cast<Modifier::Type>(i))) {
                if (i_m.isPressed(static_cast<Modifier::Type>(i)))
                    press(static_cast<Modifier::Type>(i));
                else
                    release(static_cast<Modifier::Type>(i));
            }
        }
    }
}

// stream output
std::ostream &operator<<(std::ostream &i_ost, const Modifier &i_m)
{
    struct Mods {
        Modifier::Type m_mt;
        const char *m_symbol;
    };

    const static Mods mods[] = {
        { Modifier::Type_Up, "U-" }, { Modifier::Type_Down, "D-" },
        { Modifier::Type_Shift, "S-" }, { Modifier::Type_Alt, "A-" },
        { Modifier::Type_Control, "C-" }, { Modifier::Type_Windows, "W-" },
        { Modifier::Type_Repeat, "R-" },
        { Modifier::Type_ImeLock, "IL-" },
        { Modifier::Type_ImeComp, "IC-" },
        { Modifier::Type_ImeComp, "I-" },
        { Modifier::Type_NumLock, "NL-" },
        { Modifier::Type_CapsLock, "CL-" },
        { Modifier::Type_ScrollLock, "SL-" },
        { Modifier::Type_KanaLock, "KL-" },
        { Modifier::Type_Maximized, "MAX-" },
        { Modifier::Type_Minimized, "MIN-" },
        { Modifier::Type_MdiMaximized, "MMAX-" },
        { Modifier::Type_MdiMinimized, "MMIN-" },
        { Modifier::Type_Touchpad, "T-" },
        { Modifier::Type_TouchpadSticky, "TS-" },
        { Modifier::Type_Mod0, "M0-" }, { Modifier::Type_Mod1, "M1-" },
        { Modifier::Type_Mod2, "M2-" }, { Modifier::Type_Mod3, "M3-" },
        { Modifier::Type_Mod4, "M4-" }, { Modifier::Type_Mod5, "M5-" },
        { Modifier::Type_Mod6, "M6-" }, { Modifier::Type_Mod7, "M7-" },
        { Modifier::Type_Mod8, "M8-" }, { Modifier::Type_Mod9, "M9-" },
        { Modifier::Type_Mod10, "M10-" }, { Modifier::Type_Mod11, "M11-" },
        { Modifier::Type_Mod12, "M12-" }, { Modifier::Type_Mod13, "M13-" },
        { Modifier::Type_Mod14, "M14-" }, { Modifier::Type_Mod15, "M15-" },
        { Modifier::Type_Mod16, "M16-" }, { Modifier::Type_Mod17, "M17-" },
        { Modifier::Type_Mod18, "M18-" }, { Modifier::Type_Mod19, "M19-" },
        { Modifier::Type_Lock0, "L0-" }, { Modifier::Type_Lock1, "L1-" },
        { Modifier::Type_Lock2, "L2-" }, { Modifier::Type_Lock3, "L3-" },
        { Modifier::Type_Lock4, "L4-" }, { Modifier::Type_Lock5, "L5-" },
        { Modifier::Type_Lock6, "L6-" }, { Modifier::Type_Lock7, "L7-" },
        { Modifier::Type_Lock8, "L8-" }, { Modifier::Type_Lock9, "L9-" },
    };

    for (size_t i = 0; i < NUMBER_OF(mods); ++ i)
        if (!i_m.isDontcare(mods[i].m_mt) && i_m.isPressed(mods[i].m_mt))
            i_ost << mods[i].m_symbol;
#if 0
        else if (!i_m.isDontcare(mods[i].m_mt) && i_m.isPressed(mods[i].m_mt))
            i_ost << "~" << mods[i].m_symbol;
        else
            i_ost << "*" << mods[i].m_symbol;
#endif

    return i_ost;
}


/// stream output
std::ostream &operator<<(std::ostream &i_ost, Modifier::Type i_type)
{
    const char *modNames[] = {
        "Shift",
        "Alt",
        "Control",
        "Windows",
        "Up",
        "Down",
        "Repeat",
        "ImeLock",
        "ImeComp",
        "NumLock",
        "CapsLock",
        "ScrollLock",
        "KanaLock",
        "Maximized",
        "Minimized",
        "MdiMaximized",
        "MdiMinimized",
        "Touchpad",
        "TouchpadSticky",
        "Mod0",
        "Mod1",
        "Mod2",
        "Mod3",
        "Mod4",
        "Mod5",
        "Mod6",
        "Mod7",
        "Mod8",
        "Mod9",
        "Mod10",
        "Mod11",
        "Mod12",
        "Mod13",
        "Mod14",
        "Mod15",
        "Mod16",
        "Mod17",
        "Mod18",
        "Mod19",
        "Lock0",
        "Lock1",
        "Lock2",
        "Lock3",
        "Lock4",
        "Lock5",
        "Lock6",
        "Lock7",
        "Lock8",
        "Lock9",
    };

    int i = static_cast<int>(i_type);
    if (0 <= i && i < (int)NUMBER_OF(modNames))
        i_ost << modNames[i];

    return i_ost;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ModifiedKey


// stream output
std::ostream &operator<<(std::ostream &i_ost, const ModifiedKey &i_mk)
{
    if (i_mk.m_key)
        i_ost << i_mk.m_modifier << *i_mk.m_key;
    return i_ost;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Keyboard::KeyIterator


Keyboard::KeyIterator::KeyIterator(gsl::span<Keys> i_hashedKeys)
        : m_hashedKeys(i_hashedKeys),
        m_i(m_hashedKeys.empty() ? Keys{}.begin() : m_hashedKeys[0].begin())
{
    if (!m_hashedKeys.empty() && m_hashedKeys[0].empty()) {
        size_t index = 1;
        while (index < m_hashedKeys.size() && m_hashedKeys[index].empty()) {
            ++index;
        }
        if (index < m_hashedKeys.size()) {
            m_hashedKeys = m_hashedKeys.subspan(index);
            m_i = m_hashedKeys[0].begin();
        } else {
            m_hashedKeys = gsl::span<Keys>();
        }
    }
}


void Keyboard::KeyIterator::next()
{
    if (m_hashedKeys.empty())
        return;
    ++m_i;
    if (m_i == m_hashedKeys[0].end()) {
        m_hashedKeys = m_hashedKeys.subspan(1);
        while (!m_hashedKeys.empty() && m_hashedKeys[0].empty()) {
            m_hashedKeys = m_hashedKeys.subspan(1);
        }
        if (!m_hashedKeys.empty())
            m_i = m_hashedKeys[0].begin();
    }
}


Key *Keyboard::KeyIterator::operator *()
{
    if (m_hashedKeys.empty())
        return nullptr;
    return &*m_i;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Keyboard


Keyboard::Keys &Keyboard::getKeys(const Key &i_key)
{
    ASSERT(1 <= i_key.getScanCodesSize());
    return m_hashedKeys[i_key.getScanCodes()->m_scan % HASHED_KEYS_SIZE];
}


// add a key
void Keyboard::addKey(const Key &i_key)
{
    getKeys(i_key).push_front(i_key);
}


// add a key name alias
void Keyboard::addAlias(const std::string &i_aliasName, Key *i_key)
{
    m_aliases.insert(Aliases::value_type(i_aliasName, i_key));
}

// add substitute
void Keyboard::addSubstitute(const ModifiedKey &i_mkeyFrom,
                             const ModifiedKey &i_mkeyTo)
{
    m_substitutes.push_front(Substitute(i_mkeyFrom, i_mkeyTo));
}


// add number modifier
void Keyboard::addNumberModifier(Key *i_numberKey, Key *i_modifierKey)
{
    m_numberModifiers.push_front(NumberModifier(i_numberKey, i_modifierKey));
}


// add a modifier key
void Keyboard::addModifier(Modifier::Type i_mt, Key *i_key)
{
    ASSERT((int)i_mt < (int)Modifier::Type_BASIC);
    if (std::find(m_mods[i_mt].begin(), m_mods[i_mt].end(), i_key)
            != m_mods[i_mt].end())
        return; // already added
    m_mods[i_mt].push_back(i_key);
}


// search a key
Key *Keyboard::searchKey(const Key &i_key)
{
    Keys &keys = getKeys(i_key);
    for (Keys::iterator i = keys.begin(); i != keys.end(); ++ i)
        if ((*i).isSameScanCode(i_key))
            return &*i;
    return nullptr;
}


// search a key (of which the key's scan code is the prefix)
Key *Keyboard::searchPrefixKey(const Key &i_key)
{
    Keys &keys = getKeys(i_key);
    for (Keys::iterator i = keys.begin(); i != keys.end(); ++ i)
        if ((*i).isPrefixScanCode(i_key))
            return &*i;
    return nullptr;
}


// search a key by name
Key *Keyboard::searchKey(const std::string &i_name)
{
    Aliases::iterator i = m_aliases.find(i_name);
    if (i != m_aliases.end())
        return (*i).second;
    return searchKeyByNonAliasName(i_name);
}


// search a key by non-alias name
Key *Keyboard::searchKeyByNonAliasName(const std::string &i_name)
{
    for (int j = 0; j < HASHED_KEYS_SIZE; ++ j) {
        Keys &keys = m_hashedKeys[j];
        Keys::iterator i = std::find(keys.begin(), keys.end(), i_name);
        if (i != keys.end())
            return &*i;
    }
    return nullptr;
}

/// search a substitute
ModifiedKey Keyboard::searchSubstitute(const ModifiedKey &i_mkey)
{
    for (Substitutes::const_iterator
            i = m_substitutes.begin(); i != m_substitutes.end(); ++ i)
        if (i->m_mkeyFrom.m_key == i_mkey.m_key &&
                i->m_mkeyFrom.m_modifier.doesMatch(i_mkey.m_modifier))
            return i->m_mkeyTo;
    return ModifiedKey();                // not found (.m_mkey is nullptr)
}
