//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_log.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "stringtool.h"
#include "windowstool.h"

#include <iomanip>
#include <string>
#include <sstream>


// output to m_log
void Engine::outputToLog(const Key *i_key, const ModifiedKey &i_mkey,
                         int i_debugLevel)
{
    size_t i;
    Acquire a(&m_log, i_debugLevel);

    // output scan codes
    for (i = 0; i < i_key->getScanCodesSize(); ++ i) {
        if (i_key->getScanCodes()[i].m_flags & ScanCode::E0) m_log << "E0-";
        if (i_key->getScanCodes()[i].m_flags & ScanCode::E1) m_log << "E1-";
        if (!(i_key->getScanCodes()[i].m_flags & ScanCode::E0E1))
            m_log << "   ";
        m_log << "0x" << std::hex << std::setw(2)
        << std::setfill(static_cast<tostream::char_type>('0'))
        << static_cast<int>(i_key->getScanCodes()[i].m_scan)
        << std::dec << " ";
    }

    if (!i_mkey.m_key) { // key corresponds to no phisical key
        m_log << std::endl;
        return;
    }

    // Output ModifiedKey (operator<< is defined for narrow streams only)
    {
        std::stringstream ss;
        ss << i_mkey;
        m_log << "  " << to_tstring(ss.str()) << std::endl;
    }

    // NOTE: Old investigate mode logging disabled - now using journey event format
    // The journey logging provides much more detailed information:
    // - Input evdev code and key name
    // - YAMY scan codes (input/output)
    // - Substitution visualization
    // - Output evdev code and key name
    // - End-to-end latency measurement
    // - Device identification
    //
    // if (m_isInvestigateMode && m_ipcChannel && m_ipcChannel->isConnected()) {
    //     std::stringstream ss;
    //     ss << i_mkey;
    //     std::string logLine = ss.str();
    //
    //     yamy::ipc::KeyEventNotification notification;
    //     strncpy(notification.keyEvent, logLine.c_str(), sizeof(notification.keyEvent) - 1);
    //     notification.keyEvent[sizeof(notification.keyEvent) - 1] = '\0';
    //
    //     yamy::ipc::Message message;
    //     message.type = yamy::ipc::NtfKeyEvent;
    //     message.data = &notification;
    //     message.size = sizeof(notification);
    //
    //     m_ipcChannel->send(message);
    // }
}


// describe bindings
void Engine::describeBindings()
{
    Acquire a(&m_log, 0);

    Keymap::DescribeParam dp;
    tstringstream ss;
    for (KeymapPtrList::iterator i = m_currentFocusOfThread->m_keymaps.begin();
            i != m_currentFocusOfThread->m_keymaps.end(); ++ i)
        (*i)->describe(ss, &dp);
    m_log << ss.str() << std::endl;
}


// get help message
void Engine::getHelpMessages(std::string *o_helpMessage, std::string *o_helpTitle) {
    Acquire a(&m_cs);
    *o_helpMessage = m_helpMessage;
    *o_helpTitle = m_helpTitle;
}
