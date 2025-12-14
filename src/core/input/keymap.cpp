//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// setting.cpp


#include "keymap.h"
#include "errormessage.h"
#include "stringtool.h"
#include "setting.h"
#include <algorithm>
#include <gsl/gsl>


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Action

tostream &operator<<(tostream &i_ost, const Action &i_action)
{
    return i_action.output(i_ost);
}


ActionKey::ActionKey(const ModifiedKey &i_mk)
        : m_modifiedKey(i_mk)
{
}

Action::Type ActionKey::getType() const
{
    return Type_key;
}

Action *ActionKey::clone() const
{
    Action *result = new ActionKey(m_modifiedKey);
    Ensures(result != nullptr);
    return result;
}

tostream &ActionKey::output(tostream &i_ost) const
{
    // ModifiedKey operator<< is only defined for narrow streams
    std::stringstream ss;
    ss << m_modifiedKey;
    i_ost << to_tstring(ss.str());
    return i_ost;
}

ActionKeySeq::ActionKeySeq(KeySeq *i_keySeq)
        : m_keySeq(i_keySeq)
{
}

Action::Type ActionKeySeq::getType() const
{
    return Type_keySeq;
}

// create clone
Action *ActionKeySeq::clone() const
{
    Action *result = new ActionKeySeq(m_keySeq);
    Ensures(result != nullptr);
    return result;
}

// stream output
tostream &ActionKeySeq::output(tostream &i_ost) const
{
    return i_ost << "$" << to_tstring(m_keySeq->getName());
}

ActionFunction::ActionFunction(FunctionData *i_functionData,
                               Modifier i_modifier)
        : m_functionData(i_functionData),
        m_modifier(i_modifier)
{
}

ActionFunction::~ActionFunction()
{
    delete m_functionData;
}

Action::Type ActionFunction::getType() const
{
    return Type_function;
}

// create clone
Action *ActionFunction::clone() const
{
    Action *result = new ActionFunction(m_functionData->clone(), m_modifier);
    Ensures(result != nullptr);
    return result;
}

// stream output
tostream &ActionFunction::output(tostream &i_ost) const
{
    // Modifier operator<< is only defined for narrow streams
    std::stringstream ss;
    ss << m_modifier;
    i_ost << to_tstring(ss.str()) << m_functionData;
    return i_ost;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// KeySeq


void KeySeq::copy()
{
    for (Actions::iterator i = m_actions.begin(); i != m_actions.end(); ++ i)
        (*i).reset((*i)->clone());
}


void KeySeq::clear()
{
    m_actions.clear();
}


KeySeq::KeySeq(const std::string &i_name)
        : m_name(i_name),
        m_mode(Modifier::Type_KEYSEQ)
{
}


KeySeq::KeySeq(const KeySeq &i_ks)
        : m_name(i_ks.m_name),
        m_mode(i_ks.m_mode)
{
    // Deep copy actions
    m_actions.reserve(i_ks.m_actions.size());
    for (const auto& action : i_ks.m_actions) {
        m_actions.push_back(std::unique_ptr<Action>(action->clone()));
    }
}


KeySeq::~KeySeq()
{
    clear();
}


KeySeq &KeySeq::operator=(const KeySeq &i_ks)
{
    if (this != &i_ks) {
        clear();
        m_name = i_ks.m_name;
        m_mode = i_ks.m_mode;
        
        // Deep copy actions
        m_actions.reserve(i_ks.m_actions.size());
        for (const auto& action : i_ks.m_actions) {
            m_actions.push_back(std::unique_ptr<Action>(action->clone()));
        }
    }
    return *this;
}


KeySeq &KeySeq::add(const Action &i_action)
{
    m_actions.push_back(std::unique_ptr<Action>(i_action.clone()));
    return *this;
}


/// get the first modified key of this key sequence
ModifiedKey KeySeq::getFirstModifiedKey() const
{
    if (0 < m_actions.size()) {
        const Action *a = m_actions.front().get();
        switch (a->getType()) {
        case Action::Type_key:
            return reinterpret_cast<const ActionKey *>(a)->m_modifiedKey;
        case Action::Type_keySeq:
            return reinterpret_cast<const ActionKeySeq *>(a)->
                   m_keySeq->getFirstModifiedKey();
        default:
            break;
        }
    }
    return ModifiedKey();
}


std::ostream &operator<<(std::ostream &i_ost, const KeySeq &i_ks)
{
    tstringstream tss;
    for (KeySeq::Actions::const_iterator
            i = i_ks.getActions().begin(); i != i_ks.getActions().end(); ++ i)
        tss << **i << " ";
#ifdef _WIN32
    // On Windows, tstring is wstring, need to convert to narrow
    i_ost << to_string(tss.str());
#else
    // On Linux, tstring is std::string, use directly
    i_ost << tss.str();
#endif
    return i_ost;
}

#ifdef _WIN32
tostream &operator<<(tostream &i_ost, const KeySeq &i_ks)
{
    for (KeySeq::Actions::const_iterator
            i = i_ks.getActions().begin(); i != i_ks.getActions().end(); ++ i)
        i_ost << **i << " ";
    return i_ost;
}
#endif


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Keymap


Keymap::KeyAssignments &Keymap::getKeyAssignments(const ModifiedKey &i_mk)
{
    ASSERT(1 <= i_mk.m_key->getScanCodesSize());
    return m_hashedKeyAssignments[i_mk.m_key->getScanCodes()->m_scan %
                                  HASHED_KEY_ASSIGNMENT_SIZE];
}

const Keymap::KeyAssignments &
Keymap::getKeyAssignments(const ModifiedKey &i_mk) const
{
    ASSERT(1 <= i_mk.m_key->getScanCodesSize());
    return m_hashedKeyAssignments[i_mk.m_key->getScanCodes()->m_scan %
                                  HASHED_KEY_ASSIGNMENT_SIZE];
}


Keymap::Keymap(Type i_type,
               const std::string &i_name,
               const std::string &i_windowClass,
               const std::string &i_windowTitle,
               KeySeq *i_defaultKeySeq,
               Keymap *i_parentKeymap)
        : m_type(i_type),
        m_name(i_name),
        m_windowClass(".*"),
        m_windowTitle(".*"),
        m_defaultKeySeq(i_defaultKeySeq),
        m_parentKeymap(i_parentKeymap)
{
    if (i_type == Type_windowAnd || i_type == Type_windowOr)
        try {
            std::regex::flag_type f = (std::regex::ECMAScript |
                                       std::regex::icase);
            if (!i_windowClass.empty())
                m_windowClass.assign(i_windowClass, f);
            if (!i_windowTitle.empty())
                m_windowTitle.assign(i_windowTitle, f);
        } catch (std::regex_error &i_e) {
            throw ErrorMessage() << i_e.what();
        }
}


void Keymap::addAssignment(const ModifiedKey &i_mk, KeySeq *i_keySeq)
{
    KeyAssignments &ka = getKeyAssignments(i_mk);
    for (KeyAssignments::iterator i = ka.begin(); i != ka.end(); ++ i)
        if ((*i).m_modifiedKey == i_mk) {
            (*i).m_keySeq = i_keySeq;
            return;
        }
    ka.push_front(KeyAssignment(i_mk, i_keySeq));
}


void Keymap::addModifier(Modifier::Type i_mt, AssignOperator i_ao,
                         AssignMode i_am, Key *i_key)
{
    if (i_ao == AO_new)
        m_modAssignments[i_mt].clear();
    else {
        for (ModAssignments::iterator i = m_modAssignments[i_mt].begin();
                i != m_modAssignments[i_mt].end(); ++ i)
            if ((*i).m_key == i_key) {
                (*i).m_assignOperator = i_ao;
                (*i).m_assignMode = i_am;
                return;
            }
    }
    ModAssignment ma;
    ma.m_assignOperator = i_ao;
    ma.m_assignMode = i_am;
    ma.m_key = i_key;
    m_modAssignments[i_mt].push_back(ma);
}


const Keymap::KeyAssignment *
Keymap::searchAssignment(const ModifiedKey &i_mk) const
{
    const KeyAssignments &ka = getKeyAssignments(i_mk);

    // Attempt 1: Exact match with all modifiers (including modal)
    for (KeyAssignments::const_iterator i = ka.begin(); i != ka.end(); ++ i)
        if ((*i).m_modifiedKey.m_key == i_mk.m_key &&
                (*i).m_modifiedKey.m_modifier.doesMatch(i_mk.m_modifier)) {
            const KeyAssignment *result = &(*i);
            Ensures(result != nullptr && result->m_keySeq != nullptr);
            return result;
        }

    // Attempt 2: Match without modal modifiers (standard modifiers only)
    // Create a modifier without modal modifiers (Type_Mod0..Type_Mod19)
    Modifier modWithoutModal = i_mk.m_modifier;
    for (int i = Modifier::Type_Mod0; i <= Modifier::Type_Mod19; ++i) {
        modWithoutModal.release(static_cast<Modifier::Type>(i));
    }

    for (KeyAssignments::const_iterator i = ka.begin(); i != ka.end(); ++ i)
        if ((*i).m_modifiedKey.m_key == i_mk.m_key &&
                (*i).m_modifiedKey.m_modifier.doesMatch(modWithoutModal)) {
            const KeyAssignment *result = &(*i);
            Ensures(result != nullptr && result->m_keySeq != nullptr);
            return result;
        }

    // Attempt 3: Match with no modifiers at all (base key only)
    Modifier noModifiers;
    for (KeyAssignments::const_iterator i = ka.begin(); i != ka.end(); ++ i)
        if ((*i).m_modifiedKey.m_key == i_mk.m_key &&
                (*i).m_modifiedKey.m_modifier.doesMatch(noModifiers)) {
            const KeyAssignment *result = &(*i);
            Ensures(result != nullptr && result->m_keySeq != nullptr);
            return result;
        }

    return nullptr;
}


bool Keymap::doesSameWindow(const std::string &i_className,
                            const std::string &i_titleName)
{
    if (m_type == Type_keymap)
        return false;

    std::smatch what;
    if (std::regex_search(i_className, what, m_windowClass)) {
        if (m_type == Type_windowAnd)
            return std::regex_search(i_titleName, what, m_windowTitle);
        else // type == Type_windowOr
            return true;
    } else {
        if (m_type == Type_windowAnd)
            return false;
        else // type == Type_windowOr
            return std::regex_search(i_titleName, what, m_windowTitle);
    }
}


void Keymap::adjustModifier(Keyboard &i_keyboard)
{
    for (size_t i = 0; i < NUMBER_OF(m_modAssignments); ++ i) {
        ModAssignments mos;
        if (m_parentKeymap)
            mos = m_parentKeymap->m_modAssignments[i];
        else {
            // set default modifiers
            if (i < Modifier::Type_BASIC) {
                Keyboard::Mods mods =
                    i_keyboard.getModifiers(static_cast<Modifier::Type>(i));
                for (Keyboard::Mods::iterator j = mods.begin(); j != mods.end(); ++ j) {
                    ModAssignment ma;
                    ma.m_assignOperator = AO_add;
                    ma.m_assignMode = AM_normal;
                    ma.m_key = *j;
                    mos.push_back(ma);
                }
            }
        }

        // mod adjust
        for (ModAssignments::iterator mai = m_modAssignments[i].begin();
                mai != m_modAssignments[i].end(); ++ mai) {
            ModAssignment ma = *mai;
            ma.m_assignOperator = AO_new;
            switch ((*mai).m_assignOperator) {
            case AO_new: {
                mos.clear();
                mos.push_back(ma);
                break;
            }
            case AO_add: {
                mos.push_back(ma);
                break;
            }
            case AO_sub: {
                for (ModAssignments::iterator j = mos.begin();
                        j != mos.end(); ++ j)
                    if ((*j).m_key == ma.m_key) {
                        mos.erase(j);
                        break;
                    }
                break;
            }
            case AO_overwrite: {
                for (ModAssignments::iterator j = mos.begin();
                        j != mos.end(); ++ j)
                    (*j).m_assignMode = (*mai).m_assignMode;
                break;
            }
            }
        }

        // erase redundant modifiers
        for (ModAssignments::iterator j = mos.begin(); j != mos.end(); ++ j) {
            ModAssignments::iterator k;
            for (k = j, ++ k; k != mos.end(); ++ k)
                if ((*k).m_key == (*j).m_key)
                    break;
            if (k != mos.end()) {
                k = j;
                ++ j;
                mos.erase(k);
                break;
            }
        }

        m_modAssignments[i] = mos;
    }
}


// describe
void Keymap::describe(tostream &i_ost, DescribeParam *i_dp) const
{
    // Is this keymap already described ?
    {
        DescribeParam::DescribedKeymap::iterator
        i = std::find(i_dp->m_dkeymap.begin(), i_dp->m_dkeymap.end(), this);
        if (i != i_dp->m_dkeymap.end())
            return;                    // yes!
        i_dp->m_dkeymap.push_back(this);
    }

    switch (m_type) {
    case Type_keymap:
        i_ost << "keymap " << m_name;
        break;
    case Type_windowAnd:
        i_ost << "window " << m_name << " ";
        if (m_windowTitleStr == ".*")
            i_ost << "/" << m_windowClassStr << "/";
        else
            i_ost << "( /" << m_windowClassStr << "/ && /"
            << m_windowTitleStr << "/ )";
        break;
    case Type_windowOr:
        i_ost << "window " << m_name << " ( /"
        << m_windowClassStr << "/ || /" << m_windowTitleStr
        << "/ )";
        break;
    }
    if (m_parentKeymap)
        i_ost << " : " << m_parentKeymap->m_name;
    i_ost << " = " << *m_defaultKeySeq << std::endl;

    // describe modifiers
    if (i_dp->m_doesDescribeModifiers) {
        for (int t = Modifier::Type_begin; t != Modifier::Type_end; ++ t) {
            Modifier::Type type = static_cast<Modifier::Type>(t);
            const Keymap::ModAssignments &ma = getModAssignments(type);
            if (ma.size()) {
                i_ost << " mod " << type << "\t= ";
                for (Keymap::ModAssignments::const_iterator
                        j = ma.begin(); j != ma.end(); ++ j) {
                    switch (j->m_assignMode) {
                    case Keymap::AM_true:
                        i_ost << "!";
                        break;
                    case Keymap::AM_oneShot:
                        i_ost << "!!";
                        break;
                    case Keymap::AM_oneShotRepeatable:
                        i_ost << "!!!";
                        break;
                    default:
                        break;
                    }
                    i_ost << to_tstring(j->m_key->getName()) << " ";
                }
                i_ost << std::endl;
            }
        }
        i_dp->m_doesDescribeModifiers = false;
    }

    typedef std::vector<KeyAssignment> SortedKeyAssignments;
    SortedKeyAssignments ska;
    for (size_t i = 0; i < HASHED_KEY_ASSIGNMENT_SIZE; ++ i) {
        const KeyAssignments &ka = m_hashedKeyAssignments[i];
        for (KeyAssignments::const_iterator j = ka.begin(); j != ka.end(); ++ j)
            ska.push_back(*j);
    }
    std::sort(ska.begin(), ska.end());
    for (SortedKeyAssignments::iterator i = ska.begin(); i != ska.end(); ++ i) {
        // Is this key assignment already described ?
        DescribeParam::DescribedKeys::iterator
        j = std::find(i_dp->m_dk.begin(), i_dp->m_dk.end(), i->m_modifiedKey);
        if (j != i_dp->m_dk.end())
            continue;                    // yes!

        // check if the key is an event
        Key **e;
        for (e = Event::events; *e; ++ e)
            if (i->m_modifiedKey.m_key == *e)
                break;

        // Convert Key/ModifiedKey to string using narrow stream, then to tstring
        std::ostringstream oss;
        if (*e)
            oss << " event " << *i->m_modifiedKey.m_key;
        else
            oss << " key " << i->m_modifiedKey;
        oss << "\t= " << *i->m_keySeq;
        i_ost << to_tstring(oss.str()) << std::endl;
        i_dp->m_dk.push_back(i->m_modifiedKey);
    }

    i_ost << std::endl;

    if (m_parentKeymap)
        m_parentKeymap->describe(i_ost, i_dp);
}

// set default keySeq and parent keymap if default keySeq has not been set
bool Keymap::setIfNotYet(KeySeq *i_keySeq, Keymap *i_parentKeymap)
{
    if (m_defaultKeySeq)
        return false;
    m_defaultKeySeq = i_keySeq;
    m_parentKeymap = i_parentKeymap;
    return true;
}

// stream output
extern tostream &operator<<(tostream &i_ost, const Keymap *i_keymap)
{
    return i_ost << to_tstring(i_keymap->getName());
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Keymaps


Keymaps::Keymaps()
{
}


// search by name
Keymap *Keymaps::searchByName(const std::string &i_name)
{
    for (KeymapList::iterator
            i = m_keymapList.begin(); i != m_keymapList.end(); ++ i)
        // Perform case-insensitive comparison
        if (strcasecmp_utf8(i_name.c_str(), (*i).getName().c_str()) == 0) {
            Keymap *result = &*i;
            Ensures(result != nullptr);
            return result;
        }
    return nullptr;
}


// search window
void Keymaps::searchWindow(KeymapPtrList *o_keymapPtrList,
                           const std::string &i_className,
                           const std::string &i_titleName)
{
    o_keymapPtrList->clear();
    for (KeymapList::iterator
            i = m_keymapList.begin(); i != m_keymapList.end(); ++ i)
        if ((*i).doesSameWindow(i_className, i_titleName))
            o_keymapPtrList->push_back(&(*i));
}


// add keymap
Keymap *Keymaps::add(const Keymap &i_keymap)
{
    if (Keymap *k = searchByName(i_keymap.getName()))
        return k;
    m_keymapList.push_front(i_keymap);
    Keymap *result = &m_keymapList.front();
    Ensures(result != nullptr);
    return result;
}


// adjust modifier
void Keymaps::adjustModifier(Keyboard &i_keyboard)
{
    for (KeymapList::reverse_iterator i = m_keymapList.rbegin();
            i != m_keymapList.rend(); ++ i)
        (*i).adjustModifier(i_keyboard);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// KeySeqs


// add a named keyseq (name can be empty)
KeySeq *KeySeqs::add(const KeySeq &i_keySeq)
{
    if (!i_keySeq.getName().empty()) {
        KeySeq *ks = searchByName(i_keySeq.getName());
        if (ks)
            return &(*ks = i_keySeq);
    }
    m_keySeqList.push_front(i_keySeq);
    KeySeq *result = &m_keySeqList.front();
    Ensures(result != nullptr);
    return result;
}


// search by name
KeySeq *KeySeqs::searchByName(const std::string &i_name)
{
    for (KeySeqList::iterator
            i = m_keySeqList.begin(); i != m_keySeqList.end(); ++ i)
        // Perform case-insensitive comparison
        if (strcasecmp_utf8(i_name.c_str(), (*i).getName().c_str()) == 0) {
            KeySeq *result = &*i;
            Ensures(result != nullptr);
            return result;
        }
    return nullptr;
}
