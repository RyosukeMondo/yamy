//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_setting.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>
#include <process.h>


void Engine::manageTs4mayu(TCHAR *i_ts4mayuDllName,
						   TCHAR *i_dependDllName,
						   bool i_load, HMODULE *i_pTs4mayu) {
	Acquire a(&m_log, 0);

	if (i_load == false) {
		if (*i_pTs4mayu) {
			bool (WINAPI *pTs4mayuTerm)();

			pTs4mayuTerm = (bool (WINAPI*)())GetProcAddress(*i_pTs4mayu, "ts4mayuTerm");
			if (pTs4mayuTerm() == true)
				FreeLibrary(*i_pTs4mayu);
			*i_pTs4mayu = NULL;
			m_log << i_ts4mayuDllName <<_T(" unloaded") << std::endl;
		}
	} else {
		if (*i_pTs4mayu) {
			m_log << i_ts4mayuDllName << _T(" already loaded") << std::endl;
		} else {
			if (SearchPath(NULL, i_dependDllName, NULL, 0, NULL, NULL) == 0) {
				m_log << _T("load ") << i_ts4mayuDllName
				<< _T(" failed: can't find ") << i_dependDllName
				<< std::endl;
			} else {
				*i_pTs4mayu = LoadLibrary(i_ts4mayuDllName);
				if (*i_pTs4mayu == NULL) {
					m_log << _T("load ") << i_ts4mayuDllName
					<< _T(" failed: can't find it") << std::endl;
				} else {
					bool (WINAPI *pTs4mayuInit)(UINT);

					pTs4mayuInit = (bool (WINAPI*)(UINT))GetProcAddress(*i_pTs4mayu, "ts4mayuInit");
					if (pTs4mayuInit(m_threadId) == true)
						m_log << i_ts4mayuDllName <<_T(" loaded") << std::endl;
					else
						m_log << i_ts4mayuDllName
						<<_T(" load failed: can't initialize") << std::endl;
				}
			}
		}
	}
}


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

	manageTs4mayu(_T("sts4mayu.dll"), _T("SynCOM.dll"),
				  m_setting->m_sts4mayu, &m_sts4mayu);
	manageTs4mayu(_T("cts4mayu.dll"), _T("TouchPad.dll"),
				  m_setting->m_cts4mayu, &m_cts4mayu);

	g_hookData->m_correctKanaLockHandling = m_setting->m_correctKanaLockHandling;
	if (m_currentFocusOfThread) {
		for (FocusOfThreads::iterator i = m_focusOfThreads.begin();
				i != m_focusOfThreads.end(); i ++) {
			FocusOfThread *fot = &(*i).second;
			m_setting->m_keymaps.searchWindow(&fot->m_keymaps,
											  fot->m_className, fot->m_titleName);
		}
	}
	m_setting->m_keymaps.searchWindow(&m_globalFocus.m_keymaps, _T(""), _T(""));
	if (m_globalFocus.m_keymaps.empty()) {
		Acquire a(&m_log, 0);
		m_log << _T("internal error: m_globalFocus.m_keymap is empty")
		<< std::endl;
	}
	m_currentFocusOfThread = &m_globalFocus;
	setCurrentKeymap(m_globalFocus.m_keymaps.front());
	m_hwndFocus = NULL;
	return true;
}
