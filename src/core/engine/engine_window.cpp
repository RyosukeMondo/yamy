//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_window.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>
#include <process.h>


void Engine::checkShow(HWND i_hwnd) {
	// update show style of window
	// this update should be done in hook DLL, but to
	// avoid update-loss for some applications(such as
	// cmd.exe), we update here.
	bool isMaximized = false;
	bool isMinimized = false;
	bool isMDIMaximized = false;
	bool isMDIMinimized = false;
	while (i_hwnd) {
#ifdef MAYU64
		LONG_PTR exStyle = GetWindowLongPtr(i_hwnd, GWL_EXSTYLE);
#else
		LONG exStyle = GetWindowLong(i_hwnd, GWL_EXSTYLE);
#endif
		if (exStyle & WS_EX_MDICHILD) {
			WINDOWPLACEMENT placement;
			placement.length = sizeof(WINDOWPLACEMENT);
			if (GetWindowPlacement(i_hwnd, &placement)) {
				switch (placement.showCmd) {
				case SW_SHOWMAXIMIZED:
					isMDIMaximized = true;
					break;
				case SW_SHOWMINIMIZED:
					isMDIMinimized = true;
					break;
				case SW_SHOWNORMAL:
				default:
					break;
				}
			}
		}

#ifdef MAYU64
		LONG_PTR style = GetWindowLongPtr(i_hwnd, GWL_STYLE);
#else
		LONG style = GetWindowLong(i_hwnd, GWL_STYLE);
#endif
		if ((style & WS_CHILD) == 0) {
			WINDOWPLACEMENT placement;
			placement.length = sizeof(WINDOWPLACEMENT);
			if (GetWindowPlacement(i_hwnd, &placement)) {
				switch (placement.showCmd) {
				case SW_SHOWMAXIMIZED:
					isMaximized = true;
					break;
				case SW_SHOWMINIMIZED:
					isMinimized = true;
					break;
				case SW_SHOWNORMAL:
				default:
					break;
				}
			}
		}
		i_hwnd = GetParent(i_hwnd);
	}
	setShow(isMDIMaximized, isMDIMinimized, true);
	setShow(isMaximized, isMinimized, false);
}


// lock state
bool Engine::setLockState(bool i_isNumLockToggled,
						  bool i_isCapsLockToggled,
						  bool i_isScrollLockToggled,
						  bool i_isKanaLockToggled,
						  bool i_isImeLockToggled,
						  bool i_isImeCompToggled) {
	Acquire a(&m_cs);
	if (m_isSynchronizing)
		return false;
	m_currentLock.on(Modifier::Type_NumLock, i_isNumLockToggled);
	m_currentLock.on(Modifier::Type_CapsLock, i_isCapsLockToggled);
	m_currentLock.on(Modifier::Type_ScrollLock, i_isScrollLockToggled);
	m_currentLock.on(Modifier::Type_KanaLock, i_isKanaLockToggled);
	m_currentLock.on(Modifier::Type_ImeLock, i_isImeLockToggled);
	m_currentLock.on(Modifier::Type_ImeComp, i_isImeCompToggled);
	return true;
}


// show
bool Engine::setShow(bool i_isMaximized, bool i_isMinimized,
					 bool i_isMDI) {
	Acquire a(&m_cs);
	if (m_isSynchronizing)
		return false;
	Acquire b(&m_log, 1);
	Modifier::Type max, min;
	if (i_isMDI == true) {
		max = Modifier::Type_MdiMaximized;
		min = Modifier::Type_MdiMinimized;
	} else {
		max = Modifier::Type_Maximized;
		min = Modifier::Type_Minimized;
	}
	m_currentLock.on(max, i_isMaximized);
	m_currentLock.on(min, i_isMinimized);
	m_log << _T("Set show to ") << (i_isMaximized ? _T("Maximized") :
									i_isMinimized ? _T("Minimized") : _T("Normal"));
	if (i_isMDI == true) {
		m_log << _T(" (MDI)");
	}
	m_log << std::endl;
	return true;
}


// StrExprSystem implementation
tstring Engine::getClipboardText() const
{
	return m_windowSystem->getClipboardText();
}

tstringq Engine::getStrExprWindowClassName() const
{
	return m_currentFocusOfThread->m_className;
}

tstringq Engine::getStrExprWindowTitleName() const
{
	return m_currentFocusOfThread->m_titleName;
}
