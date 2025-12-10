//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_log.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#ifdef _WIN32
#include "windowstool.h"
#endif

#include <iomanip>
#ifdef _WIN32
#include <process.h>
#endif
#include <string>


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
        m_log << "0x" << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(i_key->getScanCodes()[i].m_scan)
        << std::dec << " ";
    }

    if (!i_mkey.m_key) { // key corresponds to no phisical key
        m_log << std::endl;
        return;
    }

    m_log << "  " << i_mkey << std::endl;
}


// describe bindings
void Engine::describeBindings()
{
    Acquire a(&m_log, 0);

    Keymap::DescribeParam dp;
    for (KeymapPtrList::iterator i = m_currentFocusOfThread->m_keymaps.begin();
            i != m_currentFocusOfThread->m_keymaps.end(); ++ i)
        (*i)->describe(m_log, &dp);
    m_log << std::endl;
}


// get help message
void Engine::getHelpMessages(std::string *o_helpMessage, std::string *o_helpTitle) {
    Acquire a(&m_cs);
    *o_helpMessage = m_helpMessage;
    *o_helpTitle = m_helpTitle;
}
