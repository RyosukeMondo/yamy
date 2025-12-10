#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dlglog.h


#ifndef _DLGLOG_H
#  define _DLGLOG_H

#  include <windows.h>
#  include "msgstream.h"
#  include "../core/platform/window_system_interface.h"


//
#ifdef MAYU64
INT_PTR CALLBACK dlgLog_dlgProc(
#else
BOOL CALLBACK dlgLog_dlgProc(
#endif
    HWND i_hwnd, UINT i_message, WPARAM i_wParam, LPARAM i_lParam);

enum {
    ///
    WM_APP_dlglogNotify = WM_APP + 115,
};

enum DlgLogNotify {
    DlgLogNotify_logCleared,            ///
};

/// parameters for "Log" dialog box
class DlgLogData {
public:
    tomsgstream *m_log;                /// log stream
    HWND m_hwndTaskTray;                /// tasktray window
    yamy::platform::IWindowSystem *m_windowSystem;  /// window system abstraction
};

#endif // !_DLGLOG_H
