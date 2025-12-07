//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_log.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>
#include <process.h>


// output to m_log
void Engine::outputToLog(const Key *i_key, const ModifiedKey &i_mkey,
						 int i_debugLevel)
{
	size_t i;
	Acquire a(&m_log, i_debugLevel);

	// output scan codes
	for (i = 0; i < i_key->getScanCodesSize(); ++ i) {
		if (i_key->getScanCodes()[i].m_flags & ScanCode::E0) m_log << _T("E0-");
		if (i_key->getScanCodes()[i].m_flags & ScanCode::E1) m_log << _T("E1-");
		if (!(i_key->getScanCodes()[i].m_flags & ScanCode::E0E1))
			m_log << _T("   ");
		m_log << _T("0x") << std::hex << std::setw(2) << std::setfill(_T('0'))
		<< static_cast<int>(i_key->getScanCodes()[i].m_scan)
		<< std::dec << _T(" ");
	}

	if (!i_mkey.m_key) { // key corresponds to no phisical key
		m_log << std::endl;
		return;
	}

	m_log << _T("  ") << i_mkey << std::endl;
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
void Engine::getHelpMessages(tstring *o_helpMessage, tstring *o_helpTitle) {
	Acquire a(&m_cs);
	*o_helpMessage = m_helpMessage;
	*o_helpTitle = m_helpTitle;
}
