//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_setting.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "../platform/hook_interface.h"
#include "mayurc.h"
#ifdef _WIN32
#include "windowstool.h"
#endif

#include <iomanip>
#ifdef _WIN32
#include <process.h>
#endif
#include <string>


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
