//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_focus.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>
#include <process.h>


// check focus window
void Engine::checkFocusWindow()
{
	int count = 0;

restart:
	count ++;

	HWND hwndFore = GetForegroundWindow();
	DWORD threadId = GetWindowThreadProcessId(hwndFore, NULL);

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
						m_log << _T("RemoveThread") << std::endl;
						m_log << _T("\tHWND:\t") << std::hex << (ULONG_PTR)fot->m_hwndFocus
						<< std::dec << std::endl;
						m_log << _T("\tTHREADID:") << fot->m_threadId << std::endl;
						m_log << _T("\tCLASS:\t") << fot->m_className << std::endl;
						m_log << _T("\tTITLE:\t") << fot->m_titleName << std::endl;
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
						setCurrentKeymap(NULL);
					else
						setCurrentKeymap(*m_currentFocusOfThread->m_keymaps.begin());
					m_hwndFocus = m_currentFocusOfThread->m_hwndFocus;
					checkShow(m_hwndFocus);

					Acquire a(&m_log, 1);
					m_log << _T("FocusChanged") << std::endl;
					m_log << _T("\tHWND:\t")
					<< std::hex << (ULONG_PTR)m_currentFocusOfThread->m_hwndFocus
					<< std::dec << std::endl;
					m_log << _T("\tTHREADID:")
					<< m_currentFocusOfThread->m_threadId << std::endl;
					m_log << _T("\tCLASS:\t")
					<< m_currentFocusOfThread->m_className << std::endl;
					m_log << _T("\tTITLE:\t")
					<< m_currentFocusOfThread->m_titleName << std::endl;
					m_log << std::endl;
					return;
				}
			}
		}

		_TCHAR className[GANA_MAX_ATOM_LENGTH];
		if (GetClassName(hwndFore, className, NUMBER_OF(className))) {
			if (_tcsicmp(className, _T("ConsoleWindowClass")) == 0) {
				_TCHAR titleName[1024];
				if (GetWindowText(hwndFore, titleName, NUMBER_OF(titleName)) == 0)
					titleName[0] = _T('\0');
				setFocus(hwndFore, threadId, className, titleName, true);
				Acquire a(&m_log, 1);
				m_log << _T("HWND:\t") << std::hex << reinterpret_cast<ULONG_PTR>(hwndFore)
				<< std::dec << std::endl;
				m_log << _T("THREADID:") << threadId << std::endl;
				m_log << _T("CLASS:\t") << className << std::endl;
				m_log << _T("TITLE:\t") << titleName << std::endl << std::endl;
				goto restart;
			}
		}
	}

	Acquire a(&m_cs);
	if (m_globalFocus.m_keymaps.empty()) {
		Acquire a(&m_log, 1);
		m_log << _T("NO GLOBAL FOCUS") << std::endl;
		m_currentFocusOfThread = NULL;
		setCurrentKeymap(NULL);
	} else {
		if (m_currentFocusOfThread != &m_globalFocus) {
			Acquire a(&m_log, 1);
			m_log << _T("GLOBAL FOCUS") << std::endl;
			m_currentFocusOfThread = &m_globalFocus;
			setCurrentKeymap(m_globalFocus.m_keymaps.front());
		}
	}
	m_hwndFocus = NULL;
}


// focus
bool Engine::setFocus(HWND i_hwndFocus, DWORD i_threadId,
					  const tstringi &i_className, const tstringi &i_titleName,
					  bool i_isConsole) {
	Acquire a(&m_cs);
	if (m_isSynchronizing)
		return false;
	if (i_hwndFocus == NULL)
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
				fot->m_className == i_className &&
				fot->m_titleName == i_titleName)
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
bool Engine::threadAttachNotify(DWORD i_threadId) {
	Acquire a(&m_cs);
	m_attachedThreadIds.push_back(i_threadId);
	return true;
}


// thread detach notify
bool Engine::threadDetachNotify(DWORD i_threadId) {
	Acquire a(&m_cs);
	m_detachedThreadIds.push_back(i_threadId);
	m_attachedThreadIds.erase(remove(m_attachedThreadIds.begin(), m_attachedThreadIds.end(), i_threadId),
							  m_attachedThreadIds.end());
	return true;
}
