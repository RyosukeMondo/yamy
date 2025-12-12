//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_focus.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "stringtool.h"
#ifdef _WIN32
#include "windowstool.h"
#endif

#include <iomanip>
#include <string>
#ifdef _WIN32
#include <process.h>
#endif


// check focus window
void Engine::checkFocusWindow()
{
    int count = 0;

restart:
    count ++;

    yamy::platform::WindowHandle hwndFore = m_windowSystem->getForegroundWindow();
    uint32_t threadId = m_windowSystem->getWindowThreadId(hwndFore);

    if (hwndFore) {
        {
            Acquire a(&m_cs);
            if (m_currentFocusOfThread &&
                    m_currentFocusOfThread->m_threadId == threadId &&
                    m_currentFocusOfThread->m_hwndFocus == m_hwndFocus)
                return;

            m_emacsEditKillLine.reset();

            // erase dead thread
            if (!m_detachedThreadIds.empty()) {
                for (ThreadIds::iterator i = m_detachedThreadIds.begin();
                        i != m_detachedThreadIds.end(); i ++) {
                    FocusOfThreads::iterator j = m_focusOfThreads.find((*i));
                    if (j != m_focusOfThreads.end()) {
                        FocusOfThread *fot = &((*j).second);
                        Acquire a(&m_log, 1);
                        m_log << "RemoveThread" << std::endl;
                        m_log << "\tHWND:\t" << std::hex << reinterpret_cast<uintptr_t>(fot->m_hwndFocus)
                        << std::dec << std::endl;
                        m_log << "\tTHREADID:" << fot->m_threadId << std::endl;
                        m_log << "\tCLASS:\t" << to_tstring(fot->m_className) << std::endl;
                        m_log << "\tTITLE:\t" << to_tstring(fot->m_titleName) << std::endl;
                        m_log << std::endl;
                        m_focusOfThreads.erase(j);
                    }
                }
                m_detachedThreadIds.erase
                (m_detachedThreadIds.begin(), m_detachedThreadIds.end());
            }

            FocusOfThreads::iterator i = m_focusOfThreads.find(threadId);
            if (i != m_focusOfThreads.end()) {
                m_currentFocusOfThread = &((*i).second);
                if (!m_currentFocusOfThread->m_isConsole || 2 <= count) {
                    if (m_currentFocusOfThread->m_keymaps.empty())
                        setCurrentKeymap(nullptr);
                    else
                        setCurrentKeymap(*m_currentFocusOfThread->m_keymaps.begin());
                    m_hwndFocus = m_currentFocusOfThread->m_hwndFocus;
                    checkShow(m_hwndFocus);

                    // Debounce focus change notifications
                    auto now = std::chrono::steady_clock::now();
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastFocusChangedTime).count() > 100) {
                        notifyGUI(yamy::MessageType::FocusChanged, m_currentFocusOfThread->m_titleName);
                        m_lastFocusChangedTime = now;
                    }

                    Acquire a(&m_log, 1);
                    m_log << "FocusChanged" << std::endl;
                    m_log << "\tHWND:\t"
                    << std::hex << reinterpret_cast<uintptr_t>(m_currentFocusOfThread->m_hwndFocus)
                    << std::dec << std::endl;
                    m_log << "\tTHREADID:"
                    << m_currentFocusOfThread->m_threadId << std::endl;
                    m_log << "\tCLASS:\t"
                    << to_tstring(m_currentFocusOfThread->m_className) << std::endl;
                    m_log << "\tTITLE:\t"
                    << to_tstring(m_currentFocusOfThread->m_titleName) << std::endl;
                    m_log << std::endl;
                    return;
                }
            }
        }

        std::string className = m_windowSystem->getClassName(hwndFore);
        if (strcasecmp_utf8(className.c_str(), "ConsoleWindowClass") == 0) {
            std::string titleName = m_windowSystem->getWindowText(hwndFore);

            setFocus(hwndFore, threadId, className, titleName, true);
            Acquire a(&m_log, 1);
            m_log << "HWND:\t" << std::hex << reinterpret_cast<uintptr_t>(hwndFore)
            << std::dec << std::endl;
            m_log << "THREADID:" << threadId << std::endl;
            m_log << "CLASS:\t" << to_tstring(className) << std::endl;
            m_log << "TITLE:\t" << to_tstring(titleName) << std::endl << std::endl;
            goto restart;
        }
    }

    Acquire a(&m_cs);
    if (m_globalFocus.m_keymaps.empty()) {
        Acquire a(&m_log, 1);
        m_log << "NO GLOBAL FOCUS" << std::endl;
        m_currentFocusOfThread = nullptr;
        setCurrentKeymap(nullptr);
    } else {
        if (m_currentFocusOfThread != &m_globalFocus) {
            Acquire a(&m_log, 1);
            m_log << "GLOBAL FOCUS" << std::endl;
            m_currentFocusOfThread = &m_globalFocus;
            setCurrentKeymap(m_globalFocus.m_keymaps.front());
        }
    }
    m_hwndFocus = nullptr;
}


// focus
bool Engine::setFocus(yamy::platform::WindowHandle i_hwndFocus, uint32_t i_threadId,
                      const std::string &i_className, const std::string &i_titleName,
                      bool i_isConsole) {
    Acquire a(&m_cs);
    if (m_isSynchronizing)
        return false;
    if (i_hwndFocus == nullptr)
        return true;

    // remove newly created thread's id from m_detachedThreadIds
    if (!m_detachedThreadIds.empty()) {
        ThreadIds::iterator i;
        bool retry;
        do {
            retry = false;
            for (i = m_detachedThreadIds.begin();
                    i != m_detachedThreadIds.end(); ++ i)
                if (*i == i_threadId) {
                    m_detachedThreadIds.erase(i);
                    retry = true;
                    break;
                }
        } while (retry);
    }

    FocusOfThread *fot;
    FocusOfThreads::iterator i = m_focusOfThreads.find(i_threadId);
    if (i != m_focusOfThreads.end()) {
        fot = &(*i).second;
        if (fot->m_hwndFocus == i_hwndFocus &&
                fot->m_isConsole == i_isConsole &&
                strcasecmp_utf8(fot->m_className.c_str(), i_className.c_str()) == 0 &&
                strcasecmp_utf8(fot->m_titleName.c_str(), i_titleName.c_str()) == 0)
            return true;
    } else {
        i = m_focusOfThreads.insert(
                FocusOfThreads::value_type(i_threadId, FocusOfThread())).first;
        fot = &(*i).second;
        fot->m_threadId = i_threadId;
    }
    fot->m_hwndFocus = i_hwndFocus;
    fot->m_isConsole = i_isConsole;
    fot->m_className = i_className;
    fot->m_titleName = i_titleName;

    if (m_setting) {
        m_setting->m_keymaps.searchWindow(&fot->m_keymaps,
                                          i_className, i_titleName);
        ASSERT(0 < fot->m_keymaps.size());
    } else
        fot->m_keymaps.clear();
    checkShow(i_hwndFocus);
    return true;
}


// thread attach notify
bool Engine::threadAttachNotify(uint32_t i_threadId) {
    Acquire a(&m_cs);
    m_attachedThreadIds.push_back(i_threadId);
    return true;
}


// thread detach notify
bool Engine::threadDetachNotify(uint32_t i_threadId) {
    Acquire a(&m_cs);
    m_detachedThreadIds.push_back(i_threadId);
    m_attachedThreadIds.erase(remove(m_attachedThreadIds.begin(), m_attachedThreadIds.end(), i_threadId),
                              m_attachedThreadIds.end());
    return true;
}


// Query keymap status for a given window
Engine::KeymapStatus Engine::queryKeymapForWindow(
    yamy::platform::WindowHandle hwnd,
    const std::string& className,
    const std::string& titleName) const
{
    KeymapStatus status;
    status.isDefault = true;
    status.keymapName = "(default)";
    status.matchedClassRegex = "";
    status.matchedTitleRegex = "";
    status.activeModifiers = "";

    if (!m_setting) {
        return status;
    }

    // Search for matching keymaps
    Keymaps::KeymapPtrList keymapList;
    m_setting->m_keymaps.searchWindow(&keymapList, className, titleName);

    if (keymapList.empty()) {
        return status;
    }

    // Get the first (most specific) matched keymap
    const Keymap* keymap = keymapList.front();
    if (!keymap) {
        return status;
    }

    status.keymapName = keymap->getName();

    // Check if this is a window-specific keymap
    if (keymap->getType() == Keymap::Type_windowAnd ||
        keymap->getType() == Keymap::Type_windowOr) {
        status.isDefault = false;
        status.matchedClassRegex = keymap->getWindowClassStr();
        status.matchedTitleRegex = keymap->getWindowTitleStr();
    }

    // Build active modifiers string from current lock state
    std::string modifiers;
    if (m_currentLock.isOn(Modifier::Type_Shift)) {
        modifiers += "Shift ";
    }
    if (m_currentLock.isOn(Modifier::Type_Control)) {
        modifiers += "Ctrl ";
    }
    if (m_currentLock.isOn(Modifier::Type_Alt)) {
        modifiers += "Alt ";
    }
    if (m_currentLock.isOn(Modifier::Type_Windows)) {
        modifiers += "Win ";
    }
    if (m_currentLock.isOn(Modifier::Type_NumLock)) {
        modifiers += "NumLock ";
    }
    if (m_currentLock.isOn(Modifier::Type_CapsLock)) {
        modifiers += "CapsLock ";
    }
    if (m_currentLock.isOn(Modifier::Type_ScrollLock)) {
        modifiers += "ScrollLock ";
    }

    // Trim trailing space
    if (!modifiers.empty() && modifiers.back() == ' ') {
        modifiers.pop_back();
    }

    if (modifiers.empty()) {
        status.activeModifiers = "(none)";
    } else {
        status.activeModifiers = modifiers;
    }

    return status;
}
