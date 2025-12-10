#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dlginvestigate.h


#ifndef _DLGINVESTIGATE_H
#  define _DLGINVESTIGATE_H

#  include <windows.h>
#  include "../core/platform/window_system_interface.h"


/// dialog procedure of "Investigate" dialog box
#ifdef MAYU64
INT_PTR CALLBACK dlgInvestigate_dlgProc(
#else
BOOL CALLBACK dlgInvestigate_dlgProc(
#endif
    HWND i_hwnd, UINT i_message, WPARAM i_wParam, LPARAM i_lParam);

class Engine;

/// parameters for "Investigate" dialog box
class DlgInvestigateData {
public:
    Engine *m_engine;                /// engine
    HWND m_hwndLog;                /// log
    yamy::platform::IWindowSystem *m_windowSystem;  /// window system abstraction
};


#endif // !_DLGINVESTIGATE_H
