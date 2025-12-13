//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// mayu.cpp


#define APSTUDIO_INVOKED

#include "misc.h"
#include "compiler_specific_func.h"
#include "dlginvestigate.h"
#include "dlglog.h"
#include "dlgsetting.h"
#include "dlgversion.h"
#include "engine.h"
#include "errormessage.h"
#include "focus.h"
#include "function.h"
#include "hook.h"
#include "mayu.h"
#include "mayuipc.h"
#include "mayurc.h"
#include "msgstream.h"
#include "multithread.h"
#include "registry.h"
#include "setting.h"
#include "setting_loader.h"
#include "target.h"
#include "windowstool.h"
#include <shlwapi.h> // For Path functions
#include "fixscancodemap.h"

#include "vk2tchar.h"
#include "window_system_win32.h"
#include "input_injector_win32.h"
#include "input_hook_win32.h"
#include "input_driver_win32.h"
#include <process.h>
#include <time.h>
#include <commctrl.h>
#include <wtsapi32.h>
#include <aclapi.h>
#include <cstring>
#include <shlobj.h>
#include "../utils/debug_console.h"


///
#define ID_MENUITEM_reloadBegin _APS_NEXT_COMMAND_VALUE


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Mayu


///
class Mayu
{
    HWND m_hwndTaskTray;                /// tasktray window
    HWND m_hwndLog;                /// log dialog
    HWND m_hwndInvestigate;            /// investigate dialog
    HWND m_hwndVersion;                /// version dialog

    UINT m_WM_TaskbarRestart;            /** window message sent when
                                                    taskber restarts */
    UINT m_WM_MayuIPC;                /** IPC message sent from
                            other applications */
    NOTIFYICONDATA m_ni;                /// taskbar icon data
    HICON m_tasktrayIcon[2];            /// taskbar icon
    bool m_canUseTasktrayBaloon;            ///
    bool m_isMenuActive;                /// prevent multiple simultaneous menus

    tomsgstream m_log;                /** log stream (output to log
                            dialog's edit) */
#ifdef LOG_TO_FILE
    tofstream m_logFile;
#endif // LOG_TO_FILE

    HMENU m_hMenuTaskTray;            /// tasktray menu
#ifdef _WIN64
    HANDLE m_hMutexYamyd;
    STARTUPINFO m_si;
    PROCESS_INFORMATION m_pi;
#endif // _WIN64
    HANDLE m_mutex;
    HANDLE m_hNotifyMailslot;            /// mailslot to receive notify
    HANDLE m_hNotifyEvent;            /// event on receive notify
    OVERLAPPED m_olNotify;            ///
    BYTE m_notifyBuf[NOTIFY_MESSAGE_SIZE];
    static const DWORD SESSION_LOCKED = 1<<0;
    static const DWORD SESSION_DISCONNECTED = 1<<1;
    static const DWORD SESSION_END_QUERIED = 1<<2;
    DWORD m_sessionState;
    int m_escapeNlsKeys;
    FixScancodeMap m_fixScancodeMap;

    Setting *m_setting;                /// current setting
    bool m_isSettingDialogOpened;            /// is setting dialog opened ?

    yamy::platform::IWindowSystem *m_windowSystem;            /// window system
    ConfigStore *m_configStore;            /// config store
    yamy::platform::IInputInjector *m_inputInjector;            /// input injector
    yamy::platform::IInputHook *m_inputHook;                /// input hook
    yamy::platform::IInputDriver *m_inputDriver;            /// input driver
    Engine m_engine;                /// engine

    bool m_usingSN;           /// using WTSRegisterSessionNotification() ?
    time_t m_startTime;                /// mayu started at ...

    enum {
        WM_APP_taskTrayNotify = WM_APP + 101,    ///
        WM_APP_msgStreamNotify = WM_APP + 102,    ///
        WM_APP_escapeNLSKeysFailed = WM_APP + 121,    ///
        ID_TaskTrayIcon = 1,            ///
    };

    enum {
        YAMY_TIMER_ESCAPE_NLS_KEYS = 0,    ///
    };

private:
    static VOID CALLBACK mailslotProc(DWORD i_code, DWORD i_len, LPOVERLAPPED i_ol) {
        Mayu *pThis;

        if (i_code == ERROR_SUCCESS) {
            pThis = reinterpret_cast<Mayu*>(CONTAINING_RECORD(i_ol, Mayu, m_olNotify));
            pThis->mailslotHandler(i_code, i_len);
        }
        return;
    }

    BOOL mailslotHandler(DWORD i_code, DWORD i_len) {
        BOOL result;

        if (i_len) {
            COPYDATASTRUCT cd;

            cd.dwData = reinterpret_cast<Notify *>(m_notifyBuf)->m_type;
            cd.cbData = i_len;
            cd.lpData = m_notifyBuf;
            notifyHandler(&cd);
        }

        memset(m_notifyBuf, 0, sizeof(m_notifyBuf));
        result = ReadFileEx(m_hNotifyMailslot, m_notifyBuf, sizeof(m_notifyBuf),
                            &m_olNotify, Mayu::mailslotProc);
        return result;
    }

    /// register class for tasktray
    ATOM Register_tasktray() {
        WNDCLASS wc;
        wc.style         = 0;
        wc.lpfnWndProc   = tasktray_wndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = sizeof(Mayu *);
        wc.hInstance     = g_hInst;
        wc.hIcon         = nullptr;
        wc.hCursor       = nullptr;
        wc.hbrBackground = nullptr;
        wc.lpszMenuName  = nullptr;
        wc.lpszClassName = _T("mayuTasktray");
        return RegisterClass(&wc);
    }

    /// notify handler
    BOOL notifyHandler(COPYDATASTRUCT *cd) {
        switch (cd->dwData) {
        case Notify::Type_setFocus:
        case Notify::Type_name: {
            NotifySetFocus *n = (NotifySetFocus *)cd->lpData;
            n->m_className[NUMBER_OF(n->m_className) - 1] = _T('\0');
            n->m_titleName[NUMBER_OF(n->m_titleName) - 1] = _T('\0');

            if (n->m_type == Notify::Type_setFocus)
                m_engine.setFocus((HWND)(ULONG_PTR)n->m_hwnd, n->m_threadId,
                                  to_string(n->m_className), to_string(n->m_titleName), false);

            {
                Acquire a(&m_log, 1);
                m_log << _T("HWND:\t") << std::hex
                << n->m_hwnd
                << std::dec << std::endl;
                m_log << _T("THREADID:") << static_cast<int>(n->m_threadId)
                << std::endl;
            }
            Acquire a(&m_log, (n->m_type == Notify::Type_name) ? 0 : 1);
            m_log << _T("CLASS:\t") << n->m_className << std::endl;
            m_log << _T("TITLE:\t") << n->m_titleName << std::endl;

            bool isMDI = true;
            HWND hwnd = static_cast<HWND>(getToplevelWindow((HWND)(ULONG_PTR)n->m_hwnd, &isMDI));
            RECT rc;
            if (isMDI) {
                getChildWindowRect(hwnd, &rc);
                m_log << _T("MDI Window Position/Size: (")
                << rc.left << _T(", ") << rc.top << _T(") / (")
                << rcWidth(&rc) << _T("x") << rcHeight(&rc) << _T(")")
                << std::endl;
                hwnd = static_cast<HWND>(getToplevelWindow((HWND)(ULONG_PTR)n->m_hwnd, nullptr));
            }

            GetWindowRect(hwnd, &rc);
            m_log << _T("Toplevel Window Position/Size: (")
            << rc.left << _T(", ") << rc.top << _T(") / (")
            << rcWidth(&rc) << _T("x") << rcHeight(&rc) << _T(")")
            << std::endl;

            SystemParametersInfo(SPI_GETWORKAREA, 0, (void *)&rc, FALSE);
            m_log << _T("Desktop Window Position/Size: (")
            << rc.left << _T(", ") << rc.top << _T(") / (")
            << rcWidth(&rc) << _T("x") << rcHeight(&rc) << _T(")")
            << std::endl;

            m_log << std::endl;
            break;
        }

        case Notify::Type_lockState: {
            NotifyLockState *n = (NotifyLockState *)cd->lpData;
            m_engine.setLockState(n->m_isNumLockToggled,
                                  n->m_isCapsLockToggled,
                                  n->m_isScrollLockToggled,
                                  n->m_isKanaLockToggled,
                                  n->m_isImeLockToggled,
                                  n->m_isImeCompToggled);
#if 0
            Acquire a(&m_log, 0);
            if (n->m_isKanaLockToggled) {
                m_log << _T("Notify::Type_lockState Kana on  : ");
            } else {
                m_log << _T("Notify::Type_lockState Kana off : ");
            }
            m_log << n->m_debugParam << ", "
            << g_hookData->m_correctKanaLockHandling << std::endl;
#endif
            break;
        }

        case Notify::Type_sync: {
            m_engine.syncNotify();
            break;
        }

        case Notify::Type_threadAttach: {
            NotifyThreadAttach *n = (NotifyThreadAttach *)cd->lpData;
            m_engine.threadAttachNotify(n->m_threadId);
            break;
        }

        case Notify::Type_threadDetach: {
            NotifyThreadDetach *n = (NotifyThreadDetach *)cd->lpData;
            m_engine.threadDetachNotify(n->m_threadId);
            break;
        }

        case Notify::Type_command64: {
            NotifyCommand64 *n = (NotifyCommand64 *)cd->lpData;
            m_engine.commandNotify(n->m_hwnd, n->m_message,
                                   n->m_wParam, n->m_lParam);
            break;
        }

        case Notify::Type_command32: {
            NotifyCommand32 *n = (NotifyCommand32 *)cd->lpData;
            m_engine.commandNotify(n->m_hwnd, n->m_message,
                                   n->m_wParam, n->m_lParam);
            break;
        }

        case Notify::Type_show: {
            NotifyShow *n = (NotifyShow *)cd->lpData;
            switch (n->m_show) {
            case NotifyShow::Show_Maximized:
                m_engine.setShow(true, false, n->m_isMDI);
                break;
            case NotifyShow::Show_Minimized:
                m_engine.setShow(false, true, n->m_isMDI);
                break;
            case NotifyShow::Show_Normal:
            default:
                m_engine.setShow(false, false, n->m_isMDI);
                break;
            }
            break;
        }

        case Notify::Type_log: {
            Acquire a(&m_log, 1);
            NotifyLog *n = (NotifyLog *)cd->lpData;
            m_log << _T("hook log: ") << n->m_msg << std::endl;
            break;
        }
        }
        return true;
    }

    /// window procedure for tasktray
    static LRESULT CALLBACK
    tasktray_wndProc(HWND i_hwnd, UINT i_message,
                     WPARAM i_wParam, LPARAM i_lParam) {
#ifdef MAYU64
        Mayu *This = reinterpret_cast<Mayu *>(GetWindowLongPtr(i_hwnd, 0));
#else
        Mayu *This = reinterpret_cast<Mayu *>(GetWindowLong(i_hwnd, 0));
#endif

        if (!This)
            switch (i_message) {
            case WM_CREATE:
                This = reinterpret_cast<Mayu *>(
                           reinterpret_cast<CREATESTRUCT *>(i_lParam)->lpCreateParams);
                This->m_fixScancodeMap.init(i_hwnd, WM_APP_escapeNLSKeysFailed);
                if (This->m_escapeNlsKeys) {
                    This->m_fixScancodeMap.escape(true);
                }
#ifdef MAYU64
                SetWindowLongPtr(i_hwnd, 0, (LONG_PTR)This);
#else
                SetWindowLong(i_hwnd, 0, (long)(uintptr_t)This);
#endif
                return 0;
            }
        else
            switch (i_message) {
            case WM_COPYDATA: {
                COPYDATASTRUCT *cd;
                cd = reinterpret_cast<COPYDATASTRUCT *>(i_lParam);
                return This->notifyHandler(cd);
            }
            case WM_QUERYENDSESSION:
                if (!This->m_sessionState) {
                    if (This->m_escapeNlsKeys && This->m_engine.getIsEnabled()) {
                        This->m_fixScancodeMap.escape(false);
                    }
                }
                This->m_sessionState |= Mayu::SESSION_END_QUERIED;
                This->m_engine.prepairQuit();
                PostQuitMessage(0);
                return TRUE;

#ifndef WM_WTSSESSION_CHANGE            // WinUser.h
#  define WM_WTSSESSION_CHANGE            0x02B1
#endif
            case WM_WTSSESSION_CHANGE: {
                const char *m = "";
                switch (i_wParam) {
#ifndef WTS_CONSOLE_CONNECT            // WinUser.h
#  define WTS_CONSOLE_CONNECT                0x1
#  define WTS_CONSOLE_DISCONNECT             0x2
#  define WTS_REMOTE_CONNECT                 0x3
#  define WTS_REMOTE_DISCONNECT              0x4
#  define WTS_SESSION_LOGON                  0x5
#  define WTS_SESSION_LOGOFF                 0x6
#  define WTS_SESSION_LOCK                   0x7
#  define WTS_SESSION_UNLOCK                 0x8
#endif
                /*
                    restore NLS keys when any bits of m_sessionState is on
                    and
                    escape NLS keys when all bits of m_sessionState cleared
                */
                case WTS_CONSOLE_CONNECT:
                    This->m_sessionState &= ~Mayu::SESSION_DISCONNECTED;
                    if (!This->m_sessionState) {
                        if (This->m_escapeNlsKeys && This->m_engine.getIsEnabled()) {
                            This->m_fixScancodeMap.escape(true);
                        }
                    }
                    m = "WTS_CONSOLE_CONNECT";
                    break;
                case WTS_CONSOLE_DISCONNECT:
                    if (!This->m_sessionState) {
                        if (This->m_escapeNlsKeys && This->m_engine.getIsEnabled()) {
                            This->m_fixScancodeMap.escape(false);
                        }
                    }
                    This->m_sessionState |= Mayu::SESSION_DISCONNECTED;
                    m = "WTS_CONSOLE_DISCONNECT";
                    break;
                case WTS_REMOTE_CONNECT:
                    This->m_sessionState &= ~Mayu::SESSION_DISCONNECTED;
                    if (!This->m_sessionState) {
                        if (This->m_escapeNlsKeys && This->m_engine.getIsEnabled()) {
                            This->m_fixScancodeMap.escape(true);
                        }
                    }
                    m = "WTS_REMOTE_CONNECT";
                    break;
                case WTS_REMOTE_DISCONNECT:
                    if (!This->m_sessionState) {
                        if (This->m_escapeNlsKeys && This->m_engine.getIsEnabled()) {
                            This->m_fixScancodeMap.escape(false);
                        }
                    }
                    This->m_sessionState |= Mayu::SESSION_DISCONNECTED;
                    m = "WTS_REMOTE_DISCONNECT";
                    break;
                case WTS_SESSION_LOGON:
                    m = "WTS_SESSION_LOGON";
                    break;
                case WTS_SESSION_LOGOFF:
                    m = "WTS_SESSION_LOGOFF";
                    break;
                case WTS_SESSION_LOCK: {
                    if (!This->m_sessionState) {
                        if (This->m_escapeNlsKeys && This->m_engine.getIsEnabled()) {
                            This->m_fixScancodeMap.escape(false);
                        }
                    }
                    This->m_sessionState |= Mayu::SESSION_LOCKED;
                    m = "WTS_SESSION_LOCK";
                    break;
               }
                case WTS_SESSION_UNLOCK: {
                    This->m_sessionState &= ~Mayu::SESSION_LOCKED;
                    if (!This->m_sessionState) {
                        if (This->m_escapeNlsKeys && This->m_engine.getIsEnabled()) {
                            This->m_fixScancodeMap.escape(true);
                        }
                    }
                    m = "WTS_SESSION_UNLOCK";
                    break;
                }
                    //case WTS_SESSION_REMOTE_CONTROL: m = "WTS_SESSION_REMOTE_CONTROL"; break;
                }
                This->m_log << _T("WM_WTSESSION_CHANGE(")
                << i_wParam << ", " << i_lParam << "): "
                << m << std::endl;
                return TRUE;
            }
            case WM_APP_msgStreamNotify: {
                tomsgstream::StreamBuf *log =
                    reinterpret_cast<tomsgstream::StreamBuf *>(i_lParam);
                const tstring &str = log->acquireString();
#ifdef LOG_TO_FILE
                This->m_logFile << str << std::flush;
#endif // LOG_TO_FILE
                editInsertTextAtLast(GetDlgItem(This->m_hwndLog, IDC_EDIT_log),
                                     str, 65000);
                log->releaseString();
                return 0;
            }

            case WM_APP_taskTrayNotify: {
#ifdef _WIN32
                yamy::debug::DebugConsole::LogInfo("Tray icon event received: wParam=" + std::to_string(i_wParam) + " lParam=" + std::to_string(i_lParam));
#endif
                if (i_wParam == ID_TaskTrayIcon)
                    switch (i_lParam) {
                    case WM_RBUTTONUP: {
#ifdef _WIN32
                        yamy::debug::DebugConsole::LogInfo("Tray icon: Right-click detected, showing menu...");
#endif
                        // Prevent multiple simultaneous menus
                        if (This->m_isMenuActive) {
#ifdef _WIN32
                            yamy::debug::DebugConsole::LogWarning("Menu already active, ignoring right-click");
#endif
                            break;
                        }

                        POINT p;
                        if (!GetCursorPos(&p)) {
#ifdef _WIN32
                            yamy::debug::DebugConsole::LogError("GetCursorPos failed! Error: " + std::to_string(GetLastError()));
#endif
                            break;
                        }
#ifdef _WIN32
                        yamy::debug::DebugConsole::LogInfo("Cursor position: " + std::to_string(p.x) + ", " + std::to_string(p.y));
#endif

                        if (!SetForegroundWindow(i_hwnd)) {
#ifdef _WIN32
                            yamy::debug::DebugConsole::LogWarning("SetForegroundWindow failed! Error: " + std::to_string(GetLastError()));
#endif
                        }

#ifdef _WIN32
                        yamy::debug::DebugConsole::LogInfo("Menu handle: " + std::to_string((uintptr_t)This->m_hMenuTaskTray));
#endif
                        if (!This->m_hMenuTaskTray) {
#ifdef _WIN32
                            yamy::debug::DebugConsole::LogError("m_hMenuTaskTray is NULL! Menu not loaded!");
#endif
                            break;
                        }

                        HMENU hMenuSub = GetSubMenu(This->m_hMenuTaskTray, 0);
#ifdef _WIN32
                        yamy::debug::DebugConsole::LogInfo("Submenu handle: " + std::to_string((uintptr_t)hMenuSub));
#endif
                        if (!hMenuSub) {
#ifdef _WIN32
                            yamy::debug::DebugConsole::LogError("GetSubMenu failed! Menu has no submenu at index 0!");
#endif
                            break;
                        }
                        if (This->m_engine.getIsEnabled())
                            CheckMenuItem(hMenuSub, ID_MENUITEM_disable,
                                          MF_UNCHECKED | MF_BYCOMMAND);
                        else
                            CheckMenuItem(hMenuSub, ID_MENUITEM_disable,
                                          MF_CHECKED | MF_BYCOMMAND);
                        CHECK_TRUE( SetMenuDefaultItem(hMenuSub,
                                                       ID_MENUITEM_investigate, FALSE) );

                        // Build reload submenu (optimized - no regex!)
                        HMENU hMenuSubSub = GetSubMenu(hMenuSub, 1);
                        int mayuIndex;
                        This->m_configStore->read(to_string(_T(".mayuIndex")), &mayuIndex, 0);
                        while (DeleteMenu(hMenuSubSub, 0, MF_BYPOSITION))
                            ;
                        // Replace slow regex with simple string parsing
                        for (int index = 0; ; index ++) {
                            _TCHAR buf[100];
                            _sntprintf(buf, NUMBER_OF(buf), _T(".mayu%d"), index);
                            std::string dot_mayu;
                            if (!This->m_configStore->read(to_string(buf), &dot_mayu))
                                break;
                            // Extract name before first semicolon (replaces regex ^([^;]*);)
                            size_t semicolon_pos = dot_mayu.find(';');
                            if (semicolon_pos != std::string::npos && semicolon_pos > 0) {
                                std::string name_str = dot_mayu.substr(0, semicolon_pos);
                                MENUITEMINFO mii;
                                std::memset(&mii, 0, sizeof(mii));
                                mii.cbSize = sizeof(mii);
                                mii.fMask = MIIM_ID | MIIM_STATE | MIIM_TYPE;
                                mii.fType = MFT_STRING;
                                mii.fState =
                                    MFS_ENABLED | ((mayuIndex == index) ? MFS_CHECKED : 0);
                                mii.wID = ID_MENUITEM_reloadBegin + index;
                                tstringi name = to_tstring(name_str);
                                mii.dwTypeData = const_cast<_TCHAR *>(name.c_str());
                                mii.cch = (UINT)name.size();

                                InsertMenuItem(hMenuSubSub, index, TRUE, &mii);
                            }
                        }

                        // show popup menu
#ifdef _WIN32
                        yamy::debug::DebugConsole::LogInfo("Calling TrackPopupMenu...");
#endif
                        This->m_isMenuActive = true;
                        BOOL menuResult = TrackPopupMenu(hMenuSub, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                                       p.x, p.y, 0, i_hwnd, nullptr);
                        This->m_isMenuActive = false;
#ifdef _WIN32
                        if (!menuResult) {
                            DWORD error = GetLastError();
                            yamy::debug::DebugConsole::LogError("TrackPopupMenu failed! Error: " + std::to_string(error));
                            // ERROR_POPUP_ALREADY_ACTIVE = 1400
                            if (error == 1400) {
                                yamy::debug::DebugConsole::LogWarning("Another popup menu is already active");
                            }
                        } else {
                            yamy::debug::DebugConsole::LogInfo("TrackPopupMenu succeeded!");
                        }
#endif
                        break;
                    }

                    case WM_LBUTTONDBLCLK:
#ifdef _WIN32
                        yamy::debug::DebugConsole::LogInfo("Tray icon: Double-click detected, opening investigate dialog...");
#endif
                        SendMessage(i_hwnd, WM_COMMAND,
                                    MAKELONG(ID_MENUITEM_investigate, 0), 0);
                        break;

                    default:
#ifdef _WIN32
                        yamy::debug::DebugConsole::LogInfo("Tray icon: Unhandled message lParam=" + std::to_string(i_lParam));
#endif
                        break;
                    }
                return 0;
            }

            case WM_APP_escapeNLSKeysFailed:
                if (i_lParam) {
                    int ret;

                    This->m_log << _T("escape NLS keys done code=") << i_wParam << std::endl;
                    switch (i_wParam) {
                    case YAMY_SUCCESS:
                    case YAMY_ERROR_RETRY_INJECTION_SUCCESS:
                        // escape NLS keys success
                        break;
                    case YAMY_ERROR_TIMEOUT_INJECTION:
                        ret = This->errorDialogWithCode(IDS_escapeNlsKeysRetry, (int)i_wParam, MB_RETRYCANCEL | MB_ICONSTOP);
                        if (ret == IDRETRY) {
                            This->m_fixScancodeMap.escape(true);
                        }
                        break;
                    default:
                        This->errorDialogWithCode(IDS_escapeNlsKeysFailed, (int)i_wParam, MB_OK);
                        break;
                    }
                } else {
                    This->m_log << _T("restore NLS keys done with code=") << i_wParam << std::endl;
                }
                return 0;
                break;

            case WM_COMMAND: {
                int notify_code = HIWORD(i_wParam);
                int id = LOWORD(i_wParam);
                if (notify_code == 0) // menu
                    switch (id) {
                    default:
                        if (ID_MENUITEM_reloadBegin <= id) {
                            This->m_configStore->write(to_string(_T(".mayuIndex")), id - ID_MENUITEM_reloadBegin);
                            This->load();
                        }
                        break;
                    case ID_MENUITEM_reload:
                        This->load();
                        break;
                    case ID_MENUITEM_investigate: {
                        This->m_windowSystem->showWindow(This->m_hwndLog, SW_SHOW);
                        This->m_windowSystem->showWindow(This->m_hwndInvestigate, SW_SHOW);

                        yamy::platform::Rect rc1, rc2;
                        This->m_windowSystem->getWindowRect(This->m_hwndInvestigate, &rc1);
                        This->m_windowSystem->getWindowRect(This->m_hwndLog, &rc2);

                        yamy::platform::Rect newLogRect(rc1.left, rc1.bottom,
                                                        rc1.left + rc1.width(), rc1.bottom + rc2.height());
                        This->m_windowSystem->moveWindow(This->m_hwndLog, newLogRect);

                        This->m_windowSystem->setForegroundWindow(This->m_hwndLog);
                        This->m_windowSystem->setForegroundWindow(This->m_hwndInvestigate);
                        break;
                    }
                    case ID_MENUITEM_setting:
                        if (!This->m_isSettingDialogOpened) {
                            This->m_isSettingDialogOpened = true;
                            if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_setting),
                                          nullptr, (DLGPROC)dlgSetting_dlgProc))
                                This->load();
                            This->m_isSettingDialogOpened = false;
                        }
                        break;
                    case ID_MENUITEM_log:
                        This->m_windowSystem->showWindow(This->m_hwndLog, SW_SHOW);
                        This->m_windowSystem->setForegroundWindow(This->m_hwndLog);
                        break;
                    case ID_MENUITEM_check: {
                        BOOL ret;
                        BYTE keys[256];
                        ret = GetKeyboardState(keys);
                        if (ret == 0) {
                            This->m_log << _T("Check Keystate Failed(%d)")
                            << GetLastError() << std::endl;
                        } else {
                            This->m_log << _T("Check Keystate: ") << std::endl;
                            for (int i = 0; i < 0xff; i++) {
                                USHORT asyncKey;
                                asyncKey = GetAsyncKeyState(i);
                                This->m_log << std::hex;
                                if (asyncKey & 0x8000) {
                                    This->m_log << _T("  ") << VK2TCHAR[i]
                                    << _T("(0x") << i << _T("): pressed!")
                                    << std::endl;
                                }
                                if (i == 0x14 || // VK_CAPTITAL
                                        i == 0x15 || // VK_KANA
                                        i == 0x19 || // VK_KANJI
                                        i == 0x90 || // VK_NUMLOCK
                                        i == 0x91    // VK_SCROLL
                                   ) {
                                    if (keys[i] & 1) {
                                        This->m_log << _T("  ") << VK2TCHAR[i]
                                        << _T("(0x") << i << _T("): locked!")
                                        << std::endl;
                                    }
                                }
                                This->m_log << std::dec;
                            }
                            This->m_log << std::endl;
                        }
                        break;
                    }
                    case ID_MENUITEM_version:
                        This->m_windowSystem->showWindow(This->m_hwndVersion, SW_SHOW);
                        This->m_windowSystem->setForegroundWindow(This->m_hwndVersion);
                        break;
                    case ID_MENUITEM_help: {
                        _TCHAR buf[GANA_MAX_PATH];
                        CHECK_TRUE( GetModuleFileName(g_hInst, buf, NUMBER_OF(buf)) );
                        tstringi helpFilename = pathRemoveFileSpec(buf);
                        helpFilename += _T("\\");
                        helpFilename += to_tstring(loadString(IDS_helpFilename));
                        ShellExecute(nullptr, _T("open"), helpFilename.c_str(),
                                     nullptr, nullptr, SW_SHOWNORMAL);
                        break;
                    }
                    case ID_MENUITEM_disable:
                        This->m_engine.enable(!This->m_engine.getIsEnabled());
                        if (This->m_engine.getIsEnabled()) {
                            This->m_fixScancodeMap.escape(true);
                        } else {
                            This->m_fixScancodeMap.escape(false);
                        }
                        This->showTasktrayIcon();
                        break;
                    case ID_MENUITEM_quit:
                        This->m_engine.prepairQuit();
                        PostQuitMessage(0);
                        break;
                    }
                return 0;
            }

            case WM_APP_engineNotify: {
                switch (i_wParam) {
                case EngineNotify_shellExecute:
                    This->m_engine.shellExecute();
                    break;
                case EngineNotify_loadSetting:
                    This->load();
                    break;
                case EngineNotify_helpMessage:
                    This->showHelpMessage(false);
                    if (i_lParam)
                        This->showHelpMessage(true);
                    break;
                case EngineNotify_showDlg: {
                    // show investigate/log window
                    int sw = (i_lParam & ~MayuDialogType_mask);
                    HWND hwnd = nullptr;
                    switch (static_cast<MayuDialogType>(
                                i_lParam & MayuDialogType_mask)) {
                    case MayuDialogType_investigate:
                        hwnd = This->m_hwndInvestigate;
                        break;
                    case MayuDialogType_log:
                        hwnd = This->m_hwndLog;
                        break;
                    default:
                        break;
                    }
                    if (hwnd) {
                        This->m_windowSystem->showWindow(hwnd, sw);
                        switch (sw) {
                        case SW_SHOWNORMAL:
                        case SW_SHOWMAXIMIZED:
                        case SW_SHOW:
                        case SW_RESTORE:
                        case SW_SHOWDEFAULT:
                            This->m_windowSystem->setForegroundWindow(hwnd);
                            break;
                        }
                    }
                    break;
                }
                case EngineNotify_setForegroundWindow:
                    // FIXME: completely useless. why ?
                    setForegroundWindow(reinterpret_cast<HWND>(i_lParam));
                    {
                        Acquire a(&This->m_log, 1);
                        This->m_log << _T("setForegroundWindow(0x")
                        << std::hex << i_lParam << std::dec << _T(")")
                        << std::endl;
                    }
                    break;
                case EngineNotify_clearLog:
                    SendMessage(This->m_hwndLog, WM_COMMAND,
                                MAKELONG(IDC_BUTTON_clearLog, 0), 0);
                    break;
                default:
                    break;
                }
                return 0;
            }

            case WM_APP_dlglogNotify: {
                switch (i_wParam) {
                case DlgLogNotify_logCleared:
                    This->showBanner(true);
                    break;
                default:
                    break;
                }
                return 0;
            }

            case WM_DESTROY:
                if (This->m_usingSN) {
                    wtsUnRegisterSessionNotification(i_hwnd);
                    This->m_usingSN = false;
                }
                if (!This->m_sessionState) {
                    if (This->m_escapeNlsKeys && This->m_engine.getIsEnabled()) {
                        This->m_fixScancodeMap.escape(false);
                    }
                }
                return 0;

            default:
                if (i_message == This->m_WM_TaskbarRestart) {
                    if (This->showTasktrayIcon(true)) {
                        Acquire a(&This->m_log, 0);
                        This->m_log << _T("Tasktray icon is updated.") << std::endl;
                    } else {
                        Acquire a(&This->m_log, 1);
                        This->m_log << _T("Tasktray icon already exists.") << std::endl;
                    }
                    return 0;
                } else if (i_message == This->m_WM_MayuIPC) {
                    switch (static_cast<MayuIPCCommand>(i_wParam)) {
                    case MayuIPCCommand_Enable:
                        This->m_engine.enable(!!i_lParam);
                        if (This->m_engine.getIsEnabled()) {
                            This->m_fixScancodeMap.escape(true);
                        } else {
                            This->m_fixScancodeMap.escape(false);
                        }
                        This->showTasktrayIcon();
                        if (i_lParam) {
                            Acquire a(&This->m_log, 1);
                            This->m_log << _T("Enabled by another application.")
                            << std::endl;
                        } else {
                            Acquire a(&This->m_log, 1);
                            This->m_log << _T("Disabled by another application.")
                            << std::endl;
                        }
                        break;
                    }
                }
            }
        return DefWindowProc(i_hwnd, i_message, i_wParam, i_lParam);
    }

    /// load setting
    void load() {
        Setting *newSetting = new Setting;

        // set symbol
        for (int i = 1; i < __argc; ++ i) {
            if (__targv[i][0] == _T('-') && __targv[i][1] == _T('D'))
                newSetting->m_symbols.insert(to_string(__targv[i] + 2));
        }

        std::ostringstream narrowLog;
        if (!SettingLoader(&m_log, &narrowLog, m_configStore).load(newSetting)) {
            m_windowSystem->showWindow(m_hwndLog, SW_SHOW);
            m_windowSystem->setForegroundWindow(m_hwndLog);
            delete newSetting;
            Acquire a(&m_log, 0);
            m_log << _T("error: failed to load.") << std::endl;
            return;
        }

        m_log << _T("successfully loaded.") << std::endl;
        while (!m_engine.setSetting(newSetting))
            Sleep(1000);

        delete m_setting;
        m_setting = newSetting;
    }

    // show message (a baloon from the task tray icon)
    void showHelpMessage(bool i_doesShow = true) {
        if (m_canUseTasktrayBaloon) {
            if (i_doesShow) {
                std::string helpMessage, helpTitle;
                m_engine.getHelpMessages(&helpMessage, &helpTitle);
                tstring helpMessageW = to_tstring(helpMessage);
                tstring helpTitleW = to_tstring(helpTitle);
                tcslcpy(m_ni.szInfo, helpMessageW.c_str(), NUMBER_OF(m_ni.szInfo));
                tcslcpy(m_ni.szInfoTitle, helpTitleW.c_str(),
                        NUMBER_OF(m_ni.szInfoTitle));
                m_ni.dwInfoFlags = NIIF_INFO;
            } else
                m_ni.szInfo[0] = m_ni.szInfoTitle[0] = _T('\0');
            CHECK_TRUE( Shell_NotifyIcon(NIM_MODIFY, &m_ni) );
        }
    }

    // change the task tray icon
    bool showTasktrayIcon(bool i_doesAdd = false) {
        m_ni.hIcon  = m_tasktrayIcon[m_engine.getIsEnabled() ? 1 : 0];
        m_ni.szInfo[0] = m_ni.szInfoTitle[0] = _T('\0');
        if (i_doesAdd) {
#ifdef _WIN32
            yamy::debug::DebugConsole::LogInfo("showTasktrayIcon: Adding icon to system tray...");
#endif
            // http://support.microsoft.com/kb/418138/JA/
            int guard = 60;
            for (; !Shell_NotifyIcon(NIM_ADD, &m_ni) && 0 < guard; -- guard) {
                if (Shell_NotifyIcon(NIM_MODIFY, &m_ni)) {
#ifdef _WIN32
                    yamy::debug::DebugConsole::LogInfo("showTasktrayIcon: Icon added successfully (MODIFY)");
#endif
                    return true;
                }
                Sleep(1000);                // 1sec
            }
            if (0 < guard) {
#ifdef _WIN32
                yamy::debug::DebugConsole::LogInfo("showTasktrayIcon: Icon added successfully (ADD)");
#endif
            } else {
#ifdef _WIN32
                yamy::debug::DebugConsole::LogError("showTasktrayIcon: Failed to add icon after 60 retries! Error: " + std::to_string(GetLastError()));
#endif
            }
            return 0 < guard;
        } else {
            BOOL result = Shell_NotifyIcon(NIM_MODIFY, &m_ni);
#ifdef _WIN32
            if (result) {
                yamy::debug::DebugConsole::LogInfo("showTasktrayIcon: Icon modified successfully");
            } else {
                yamy::debug::DebugConsole::LogError("showTasktrayIcon: Failed to modify icon! Error: " + std::to_string(GetLastError()));
            }
#endif
            return !!result;
        }
    }

    void showBanner(bool i_isCleared) {
        time_t now;
        time(&now);

        _TCHAR starttimebuf[1024];
        _TCHAR timebuf[1024];

#ifdef __BORLANDC__
#pragma message("\t\t****\tAfter std::ostream() is called,  ")
#pragma message("\t\t****\tstrftime(... \"%%#c\" ...) fails.")
#pragma message("\t\t****\tWhy ? Bug of Borland C++ 5.5.1 ? ")
#pragma message("\t\t****\t                         - nayuta")
        _tcsftime(timebuf, NUMBER_OF(timebuf), _T("%Y/%m/%d %H:%M:%S"),
                  localtime(&now));
        _tcsftime(starttimebuf, NUMBER_OF(starttimebuf), _T("%Y/%m/%d %H:%M:%S"),
                  localtime(&m_startTime));
#else
        _tcsftime(timebuf, NUMBER_OF(timebuf), _T("%#c"), localtime(&now));
        _tcsftime(starttimebuf, NUMBER_OF(starttimebuf), _T("%#c"),
                  localtime(&m_startTime));
#endif

        Acquire a(&m_log, 0);
        m_log << _T("------------------------------------------------------------") << std::endl;
        m_log << to_tstring(loadString(IDS_mayu)) << _T(" ") _T(VERSION);
#ifndef NDEBUG
        m_log << _T(" (DEBUG)");
#endif
#ifdef _UNICODE
        m_log << _T(" (UNICODE)");
#endif
        m_log << std::endl;
#ifndef LOGNAME
#define LOGNAME "unknown"
#endif
#ifndef COMPUTERNAME
#define COMPUTERNAME "unknown"
#endif
        m_log << _T("  built by ")
        << _T(LOGNAME) << _T("@") << to_tstring(toLower(to_string(_T(COMPUTERNAME))))
        << _T(" (") << _T(__DATE__) <<  _T(" ")
        << _T(__TIME__) << _T(", ")
        << to_tstring(getCompilerVersionString()) << _T(")") << std::endl;
        _TCHAR modulebuf[1024];
        CHECK_TRUE( GetModuleFileName(g_hInst, modulebuf,
                                      NUMBER_OF(modulebuf)) );
        m_log << _T("started at ") << starttimebuf << std::endl;
        m_log << modulebuf << std::endl;
        m_log << _T("------------------------------------------------------------") << std::endl;

        if (i_isCleared) {
            m_log << _T("log was cleared at ") << timebuf << std::endl;
        } else {
            m_log << _T("log begins at ") << timebuf << std::endl;
        }
    }

    int errorDialogWithCode(UINT ids, int code, UINT style = MB_OK | MB_ICONSTOP)
    {
        _TCHAR title[1024];
        _TCHAR text[1024];

        _sntprintf_s(title, NUMBER_OF(title), _TRUNCATE, to_tstring(loadString(IDS_mayu)).c_str());
        _sntprintf_s(text, NUMBER_OF(text), _TRUNCATE, to_tstring(loadString(ids)).c_str(), code);
         return MessageBox((HWND)nullptr, text, title, style);
    }

    int enableToWriteByUser(HANDLE hdl)
    {
        TCHAR userName[GANA_MAX_ATOM_LENGTH];
        DWORD userNameSize = NUMBER_OF(userName);

        SID_NAME_USE sidType;
        PSID pSid = nullptr;
        DWORD sidSize = 0;
        TCHAR *pDomain = nullptr;
        DWORD domainSize = 0;

        PSECURITY_DESCRIPTOR pSd;
        PACL pOrigDacl;
        ACL_SIZE_INFORMATION aclInfo;

        PACL pNewDacl = nullptr;
        DWORD newDaclSize;

        DWORD aceIndex;
        DWORD newAceIndex = 0;

        BOOL ret;
        int err = 0;

        ret = GetUserName(userName, &userNameSize);
        if (ret == FALSE) {
            err = YAMY_ERROR_ON_GET_USERNAME;
            goto exit;
        }

        // get buffer size for pSid (and pDomain)
        ret = LookupAccountName(nullptr, userName, pSid, &sidSize, pDomain, &domainSize, &sidType);
        if (ret != FALSE || GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            // above call should fail by ERROR_INSUFFICIENT_BUFFER
            err = YAMY_ERROR_ON_GET_LOGONUSERNAME;
            goto exit;
        }

        pSid = reinterpret_cast<PSID>(LocalAlloc(LPTR, sidSize));
        pDomain = reinterpret_cast<TCHAR*>(LocalAlloc(LPTR, domainSize * sizeof(TCHAR)));
        if (pSid == nullptr || pDomain == nullptr) {
            err = YAMY_ERROR_NO_MEMORY;
            goto exit;
        }

        // get SID (and Domain) for logoned user
        ret = LookupAccountName(nullptr, userName, pSid, &sidSize, pDomain, &domainSize, &sidType);
        if (ret == FALSE) {
            // LookupAccountName() should success in this time
            err = YAMY_ERROR_ON_GET_LOGONUSERNAME;
            goto exit;
        }

        // get DACL for hdl
        ret = GetSecurityInfo(hdl, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &pOrigDacl, nullptr, &pSd);
        if (ret != ERROR_SUCCESS) {
            err = YAMY_ERROR_ON_GET_SECURITYINFO;
            goto exit;
        }

        // get size for original DACL
        ret = GetAclInformation(pOrigDacl, &aclInfo, sizeof(aclInfo), AclSizeInformation);
        if (ret == FALSE) {
            err = YAMY_ERROR_ON_GET_DACL;
            goto exit;
        }

        // compute size for new DACL
        newDaclSize = aclInfo.AclBytesInUse + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSid) - sizeof(DWORD);

        // allocate memory for new DACL
        pNewDacl = reinterpret_cast<PACL>(LocalAlloc(LPTR, newDaclSize));
        if (pNewDacl == nullptr) {
            err = YAMY_ERROR_NO_MEMORY;
            goto exit;
        }

        // initialize new DACL
        ret = InitializeAcl(pNewDacl, newDaclSize, ACL_REVISION);
        if (ret == FALSE) {
            err = YAMY_ERROR_ON_INITIALIZE_ACL;
            goto exit;
        }

        // copy original DACL to new DACL
        for (aceIndex = 0; aceIndex < aclInfo.AceCount; aceIndex++) {
            LPVOID pAce;

            ret = GetAce(pOrigDacl, aceIndex, &pAce);
            if (ret == FALSE) {
                err = YAMY_ERROR_ON_GET_ACE;
                goto exit;
            }

            if ((reinterpret_cast<ACCESS_ALLOWED_ACE*>(pAce))->Header.AceFlags & INHERITED_ACE) {
                break;
            }

            if (EqualSid(pSid, &(reinterpret_cast<ACCESS_ALLOWED_ACE*>(pAce))->SidStart) != FALSE) {
                continue;
            }

            ret = AddAce(pNewDacl, ACL_REVISION, MAXDWORD, pAce, (reinterpret_cast<PACE_HEADER>(pAce))->AceSize);
            if (ret == FALSE) {
                err = YAMY_ERROR_ON_ADD_ACE;
                goto exit;
            }

            newAceIndex++;
        }

        ret = AddAccessAllowedAce(pNewDacl, ACL_REVISION, GENERIC_ALL, pSid);
        if (ret == FALSE) {
            err = YAMY_ERROR_ON_ADD_ALLOWED_ACE;
            goto exit;
        }

        // copy the rest of inherited ACEs
        for (; aceIndex < aclInfo.AceCount; aceIndex++) {
            LPVOID pAce;

            ret = GetAce(pOrigDacl, aceIndex, &pAce);
            if (ret == FALSE) {
                err = YAMY_ERROR_ON_GET_ACE;
                goto exit;
            }

            ret = AddAce(pNewDacl, ACL_REVISION, MAXDWORD, pAce, (reinterpret_cast<PACE_HEADER>(pAce))->AceSize);
            if (ret == FALSE) {
                err = YAMY_ERROR_ON_ADD_ACE;
                goto exit;
            }
        }

        ret = SetSecurityInfo(hdl, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, pNewDacl, nullptr);
        if (ret != ERROR_SUCCESS) {
            err = YAMY_ERROR_ON_SET_SECURITYINFO;
        }

exit:
        LocalFree(pSd);
        LocalFree(pSid);
        LocalFree(pDomain);
        LocalFree(pNewDacl);

        return err;
    }

public:
    ///
    Mayu(HANDLE i_mutex)
            : m_hwndTaskTray(nullptr),
            m_hwndLog(nullptr),
            m_WM_TaskbarRestart(RegisterWindowMessage(_T("TaskbarCreated"))),
            m_WM_MayuIPC(RegisterWindowMessage(WM_MayuIPC_NAME)),
            m_canUseTasktrayBaloon(
                PACKVERSION(5, 0) <= getDllVersion(_T("shlwapi.dll"))),
            m_isMenuActive(false),
            m_log(WM_APP_msgStreamNotify),
#ifdef LOG_TO_FILE
            m_logFile(_T("logs\\mayu.log")),
#endif // LOG_TO_FILE
            m_hMenuTaskTray(nullptr),
            m_mutex(i_mutex),
            m_hNotifyMailslot(INVALID_HANDLE_VALUE),
            m_hNotifyEvent(nullptr),
            m_sessionState(0),
            m_escapeNlsKeys(0),
            m_setting(nullptr),
            m_isSettingDialogOpened(false),
            m_windowSystem(new yamy::platform::WindowSystemWin32),
#ifdef USE_INI
            m_configStore(new Registry(0, to_string(_T("yamy")))),
#else
            m_configStore(new Registry(HKEY_CURRENT_USER, to_string(_T("Software\\gimy.net\\yamy")))),
#endif
            m_inputInjector(new yamy::platform::InputInjectorWin32(m_windowSystem)),
            m_inputHook(new yamy::platform::InputHookWin32),
            m_inputDriver(new yamy::platform::InputDriverWin32),
            m_engine(m_log, m_windowSystem, m_configStore, m_inputInjector, m_inputHook, m_inputDriver),
            m_usingSN(false),
            m_startTime(time(nullptr))
    {
        yamy::debug::DebugConsole::LogInfo("Mayu constructor: Initializing components...");

        yamy::debug::DebugConsole::LogInfo("Mayu: Reading config...");
        m_configStore->read(to_string(_T("escapeNLSKeys")), &m_escapeNlsKeys, 0);

        yamy::debug::DebugConsole::LogInfo("Mayu: Creating mailslot...");
        m_hNotifyMailslot = CreateMailslot(NOTIFY_MAILSLOT_NAME, 0, MAILSLOT_WAIT_FOREVER, (SECURITY_ATTRIBUTES *)nullptr);
        if (m_hNotifyMailslot == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            if (error == ERROR_ALREADY_EXISTS) {
                yamy::debug::DebugConsole::LogError("YAMY is already running. Please close the existing instance first.");
                MessageBoxA(NULL,
                    "YAMY is already running.\n\nPlease close the existing instance before starting a new one.\n\nCheck the system tray for the YAMY icon.",
                    "YAMY Already Running",
                    MB_OK | MB_ICONWARNING);
                throw std::runtime_error("YAMY already running");
            } else {
                yamy::debug::DebugConsole::CriticalError("Failed to create mailslot. Error: " + std::to_string(error));
                ASSERT(false);
            }
        }

        yamy::debug::DebugConsole::LogInfo("Mayu: Setting mailslot permissions...");
        int err;
        if (checkWindowsVersion(6, 0) != FALSE) { // enableToWriteByUser() is available only Vista or later
            err = enableToWriteByUser(m_hNotifyMailslot);
            if (err) {
                yamy::debug::DebugConsole::LogWarning("Failed to enable write permissions for mailslot. Error: " + std::to_string(err));
                errorDialogWithCode(IDS_cannotPermitStandardUser, err);
            }
        }

        yamy::debug::DebugConsole::LogInfo("Mayu: Creating notify event...");
        m_hNotifyEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!m_hNotifyEvent) {
            yamy::debug::DebugConsole::CriticalError("Failed to create notify event");
            ASSERT(false);
        }
        m_olNotify.Offset = 0;
        m_olNotify.OffsetHigh = 0;
        m_olNotify.hEvent = m_hNotifyEvent;
        time(&m_startTime);

        yamy::debug::DebugConsole::LogInfo("Mayu: Registering window classes...");
        if (!Register_focus()) {
            yamy::debug::DebugConsole::CriticalError("Failed to register focus window class");
        }
        CHECK_TRUE( Register_focus() );

        if (!Register_target()) {
            yamy::debug::DebugConsole::CriticalError("Failed to register target window class");
        }
        CHECK_TRUE( Register_target() );

        if (!Register_tasktray()) {
            yamy::debug::DebugConsole::CriticalError("Failed to register tasktray window class");
        }
        CHECK_TRUE( Register_tasktray() );

        // change dir
#if 0
        HomeDirectories pathes;
        getHomeDirectories(&pathes);
        for (HomeDirectories::iterator i = pathes.begin(); i != pathes.end(); ++ i)
            if (SetCurrentDirectory(i->c_str()))
                break;
#endif

        // create windows, dialogs
        yamy::debug::DebugConsole::LogInfo("Mayu: Creating tasktray window...");
        tstringi title = to_tstring(loadString(IDS_mayu));
        m_hwndTaskTray = CreateWindow(_T("mayuTasktray"), title.c_str(),
                                      WS_OVERLAPPEDWINDOW,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      nullptr, nullptr, g_hInst, this);
        if (!m_hwndTaskTray) {
            DWORD error = GetLastError();
            yamy::debug::DebugConsole::CriticalError("Failed to create tasktray window. Error: " + std::to_string(error));
        }
        CHECK_TRUE( m_hwndTaskTray );

        // Allow tray icon messages from lower integrity processes (UIPI fix)
        yamy::debug::DebugConsole::LogInfo("Mayu: Enabling message filter for tray icon...");
#if defined(_WIN32) && !defined(UNDER_CE)
        typedef BOOL (WINAPI *ChangeWindowMessageFilterFunc)(UINT, DWORD);
        HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
        if (hUser32) {
            ChangeWindowMessageFilterFunc pChangeWindowMessageFilter =
                (ChangeWindowMessageFilterFunc)GetProcAddress(hUser32, "ChangeWindowMessageFilter");
            if (pChangeWindowMessageFilter) {
                // MSGFLT_ADD = 1 (allow message)
                BOOL result = pChangeWindowMessageFilter(WM_APP_taskTrayNotify, 1);
                if (result) {
                    yamy::debug::DebugConsole::LogInfo("Mayu: Message filter enabled for WM_APP_taskTrayNotify");
                } else {
                    yamy::debug::DebugConsole::LogWarning("Mayu: Failed to enable message filter. Error: " + std::to_string(GetLastError()));
                }
            } else {
                yamy::debug::DebugConsole::LogInfo("Mayu: ChangeWindowMessageFilter not available (older Windows)");
            }
        }
#endif

        // set window handle of tasktray to hooks
        yamy::debug::DebugConsole::LogInfo("Mayu: Installing keyboard hooks...");
        yamy::debug::DebugConsole::LogWarning("This may be blocked by Windows Defender or antivirus!");
        BOOL hookResult = installMessageHook((DWORD)((ULONG_PTR)m_hwndTaskTray));
        if (hookResult != 0) {
            yamy::debug::DebugConsole::CriticalError(
                "Failed to install keyboard hooks. Error code: " + std::to_string(hookResult) + "\n\n" +
                "This is usually caused by:\n" +
                "1. Windows Defender Real-time Protection\n" +
                "2. Antivirus software\n" +
                "3. Missing yamy64.dll or yamy32.dll\n\n" +
                "Solution:\n" +
                "- Add YAMY folder to Windows Defender exclusions\n" +
                "- Run as Administrator\n" +
                "- Check that .dll files are in the same folder as .exe"
            );
        } else {
            yamy::debug::DebugConsole::LogInfo("Mayu: Keyboard hooks installed successfully!");
        }
        CHECK_FALSE( hookResult );

        yamy::debug::DebugConsole::LogInfo("Mayu: Registering session notification...");
        m_usingSN = wtsRegisterSessionNotification(m_hwndTaskTray,
                    NOTIFY_FOR_THIS_SESSION);

        // Skip dialog creation - these dialogs crash in administrator mode
        // They are not essential for core keyboard remapping functionality
        yamy::debug::DebugConsole::LogInfo("Mayu: Skipping debug dialog creation (not needed for core functionality)...");
        m_hwndLog = nullptr;
        m_hwndInvestigate = nullptr;
        m_hwndVersion = nullptr;
        yamy::debug::DebugConsole::LogInfo("Mayu: Debug dialogs skipped, continuing with core initialization...");

        // attach log
#ifdef LOG_TO_FILE
        tstring path;
        _TCHAR exePath[GANA_MAX_PATH];
        _TCHAR exeDrive[GANA_MAX_PATH];
        _TCHAR exeDir[GANA_MAX_PATH];
        GetModuleFileName(nullptr, exePath, GANA_MAX_PATH);
        _tsplitpath_s(exePath, exeDrive, GANA_MAX_PATH, exeDir, GANA_MAX_PATH, nullptr, 0, nullptr, 0);
        path = exeDrive;
        path += exeDir;
        path += _T("mayu.log");
        m_logFile.open(path.c_str(), std::ios::app);
        m_logFile.imbue(std::locale("japanese"));
#endif // LOG_TO_FILE
        if (m_hwndLog) {
            SendMessage(GetDlgItem(m_hwndLog, IDC_EDIT_log), EM_SETLIMITTEXT, 0, 0);
        }
        m_log.attach(m_hwndTaskTray);

        yamy::debug::DebugConsole::LogInfo("Mayu: Starting engine...");
        // start keyboard handler thread
        m_engine.setAssociatedWndow(m_hwndTaskTray);
        m_engine.start();

        yamy::debug::DebugConsole::LogInfo("Mayu: Setting up tasktray icon...");
        yamy::debug::DebugConsole::LogInfo("Mayu: Tasktray window handle: " + std::to_string((uintptr_t)m_hwndTaskTray));
        // show tasktray icon
        m_tasktrayIcon[0] = loadSmallIcon(IDI_ICON_mayu_disabled);
        m_tasktrayIcon[1] = loadSmallIcon(IDI_ICON_mayu);
        std::memset(&m_ni, 0, sizeof(m_ni));
        m_ni.uID    = ID_TaskTrayIcon;
        m_ni.hWnd   = m_hwndTaskTray;
        m_ni.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        m_ni.hIcon  = m_tasktrayIcon[1];
        m_ni.uCallbackMessage = WM_APP_taskTrayNotify;
        yamy::debug::DebugConsole::LogInfo("Mayu: NOTIFYICONDATA configured - hWnd=" + std::to_string((uintptr_t)m_ni.hWnd) +
                                          " uCallbackMessage=" + std::to_string(m_ni.uCallbackMessage));
        tstring tip = to_tstring(loadString(IDS_mayu));
        tcslcpy(m_ni.szTip, tip.c_str(), NUMBER_OF(m_ni.szTip));
        if (m_canUseTasktrayBaloon) {
            m_ni.cbSize = NOTIFYICONDATA_V3_SIZE;
            m_ni.uFlags |= NIF_INFO;
        } else
            m_ni.cbSize = NOTIFYICONDATA_V1_SIZE;

        yamy::debug::DebugConsole::LogInfo("Mayu: Showing tasktray icon...");
        showTasktrayIcon(true);

        yamy::debug::DebugConsole::LogInfo("Mayu: Creating menu...");
        // create menu
        m_hMenuTaskTray = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU_tasktray));
        ASSERT(m_hMenuTaskTray);

        yamy::debug::DebugConsole::LogInfo("Mayu: Setting lock state...");
        // set initial lock state
        notifyLockState();

#ifdef _WIN64
        yamy::debug::DebugConsole::LogInfo("Mayu: Launching yamyd32.exe (64-bit mode)...");
        ZeroMemory(&m_pi,sizeof(m_pi));
        ZeroMemory(&m_si,sizeof(m_si));
        m_si.cb=sizeof(m_si);

        // create mutex to block yamyd
        m_hMutexYamyd = CreateMutex((SECURITY_ATTRIBUTES *)nullptr, TRUE, MUTEX_YAMYD_BLOCKER);

        tstring yamydPath;
        _TCHAR exePath[GANA_MAX_PATH];
        _TCHAR exeDrive[GANA_MAX_PATH];
        _TCHAR exeDir[GANA_MAX_PATH];

        GetModuleFileName(nullptr, exePath, GANA_MAX_PATH);
        _tsplitpath_s(exePath, exeDrive, GANA_MAX_PATH, exeDir, GANA_MAX_PATH, nullptr, 0, nullptr, 0);
        yamydPath = exeDrive;
        yamydPath += exeDir;
        yamydPath += _T("yamyd32.exe");

        BOOL result = CreateProcess(yamydPath.c_str(), nullptr, nullptr, nullptr, FALSE,
                               NORMAL_PRIORITY_CLASS, 0, nullptr, &m_si, &m_pi);
        if (result == FALSE) {
            DWORD error = GetLastError();
            yamy::debug::DebugConsole::LogWarning("Failed to launch yamyd32.exe. Error: " + std::to_string(error));
            TCHAR buf[1024];
            TCHAR text[1024];
            TCHAR title[1024];

            m_pi.hProcess = nullptr;
            LoadString(GetModuleHandle(nullptr), IDS_cannotInvoke,
                       text, sizeof(text)/sizeof(text[0]));
            LoadString(GetModuleHandle(nullptr), IDS_mayu,
                       title, sizeof(title)/sizeof(title[0]));
            _stprintf_s(buf, sizeof(buf)/sizeof(buf[0]),
                        text, _T("yamyd32"), GetLastError());
             MessageBox((HWND)nullptr, buf, title, MB_OK | MB_ICONSTOP);
        } else {
            yamy::debug::DebugConsole::LogInfo("Mayu: yamyd32.exe launched successfully!");
            CloseHandle(m_pi.hThread);
        }
#endif // _WIN64

        yamy::debug::DebugConsole::LogInfo("Mayu constructor completed successfully!");
    }

    ///
    ~Mayu() {
        // stop notify from mayu.dll
        g_hookData->m_hwndTaskTray = 0;
        CHECK_FALSE( uninstallMessageHook() );

#ifdef _WIN64
        ReleaseMutex(m_hMutexYamyd);
        if (m_pi.hProcess) {
            WaitForSingleObject(m_pi.hProcess, 5000);
            CloseHandle(m_pi.hProcess);
        }
        CloseHandle(m_hMutexYamyd);
#endif // _WIN64

        CancelIo(m_hNotifyMailslot);
        SleepEx(0, TRUE);
        CloseHandle(m_hNotifyMailslot);
        CHECK_TRUE( DestroyWindow(m_hwndLog) );
        CHECK_TRUE( DestroyWindow(m_hwndTaskTray) );

        // destroy menu
        DestroyMenu(m_hMenuTaskTray);

        // delete tasktray icon
        CHECK_TRUE( Shell_NotifyIcon(NIM_DELETE, &m_ni) );
        CHECK_TRUE( DestroyIcon(m_tasktrayIcon[1]) );
        CHECK_TRUE( DestroyIcon(m_tasktrayIcon[0]) );

        // stop keyboard handler thread
        m_engine.stop();

        if (!(m_sessionState & SESSION_END_QUERIED)) {
            DWORD_PTR result;
            SendMessageTimeout(HWND_BROADCAST, WM_NULL, 0, 0, 0, 3000, &result);
        }

        // remove setting;
        delete m_inputHook;
        delete m_inputDriver;
        delete m_inputInjector;
        delete m_configStore;
        delete m_windowSystem;
    }

    /// message loop
    WPARAM messageLoop() {
        showBanner(false);
        load();

        mailslotHandler(0, 0);
        while (1) {
            HANDLE handles[] = { m_hNotifyEvent };
            DWORD ret;
            switch (ret = MsgWaitForMultipleObjectsEx(NUMBER_OF(handles), &handles[0],
                          INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE)) {
            case WAIT_OBJECT_0:            // m_hNotifyEvent
                break;

            case WAIT_OBJECT_0 + NUMBER_OF(handles): {
                MSG msg;
                if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) != 0) {
                    if (msg.message == WM_QUIT) {
                        return msg.wParam;
                    }
                    if (IsDialogMessage(m_hwndLog, &msg))
                        break;
                    if (IsDialogMessage(m_hwndInvestigate, &msg))
                        break;
                    if (IsDialogMessage(m_hwndVersion, &msg))
                        break;
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    break;
                }
                break;
            }

            case WAIT_IO_COMPLETION:
                break;

            case 0x102:
            default:
                break;
            }
        }
    }
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Functions


/// convert registry
void convertRegistry()
{
#ifdef USE_INI
    Registry reg(0, to_string(_T("yamy")));
#else
    Registry reg(HKEY_CURRENT_USER, to_string(_T("Software\\gimy.net\\yamy")));
#endif
    std::string dot_mayu;
    bool doesAdd = false;
    DWORD index;
    if (reg.read(to_string(_T(".mayu")), &dot_mayu)) {
        reg.write(to_string(_T(".mayu0")), to_string(_T(";")) + dot_mayu + to_string(_T(";")));
        reg.remove(to_string(_T(".mayu")));
        doesAdd = true;
        index = 0;
    } else if (!reg.read(to_string(_T(".mayu0")), &dot_mayu)) {
        reg.write(to_string(_T(".mayu0")), loadString(IDS_readFromHomeDirectory) + to_string(_T(";;")));
        doesAdd = true;
        index = 1;
    }
    if (doesAdd) {
        Registry commonreg(HKEY_LOCAL_MACHINE, to_string(_T("Software\\GANAware\\mayu")));
        std::string dir, layout;
        if (commonreg.read(to_string(_T("dir")), &dir) &&
                commonreg.read(to_string(_T("layout")), &layout)) {
            std::string tmp = to_string(_T(";")) + dir + to_string(_T("\\dot.mayu"));
            if (layout == to_string(_T("109"))) {
                reg.write(to_string(_T(".mayu1")), loadString(IDS_109Emacs) + tmp
                          + to_string(_T(";-DUSE109")) + to_string(_T(";-DUSEdefault")));
                reg.write(to_string(_T(".mayu2")), loadString(IDS_104on109Emacs) + tmp
                          + to_string(_T(";-DUSE109")) + to_string(_T(";-DUSEdefault")) + to_string(_T(";-DUSE104on109")));
                reg.write(to_string(_T(".mayu3")), loadString(IDS_109) + tmp
                          + to_string(_T(";-DUSE109")));
                reg.write(to_string(_T(".mayu4")), loadString(IDS_104on109) + tmp
                          + to_string(_T(";-DUSE109")) + to_string(_T(";-DUSE104on109")));
            } else {
                reg.write(to_string(_T(".mayu1")), loadString(IDS_104Emacs) + tmp
                          + to_string(_T(";-DUSE104")) + to_string(_T(";-DUSEdefault")));
                reg.write(to_string(_T(".mayu2")), loadString(IDS_109on104Emacs) + tmp
                          + to_string(_T(";-DUSE104")) + to_string(_T(";-DUSEdefault")) + to_string(_T(";-DUSE109on104")));
                reg.write(to_string(_T(".mayu3")), loadString(IDS_104) + tmp
                          + to_string(_T(";-DUSE104")));
                reg.write(to_string(_T(".mayu4")), loadString(IDS_109on104) + tmp
                          + to_string(_T(";-DUSE104")) + to_string(_T(";-DUSE109on104")));
            }
            reg.write(to_string(_T(".mayuIndex")), index);
        }
    }
}



/// main

// Forward declaration
int appMain(const std::string& cmdLine);

/// main
extern "C" int WINAPI wWinMain(HINSTANCE i_hInstance, HINSTANCE /* i_hPrevInstance */,
                     LPWSTR i_lpszCmdLine, int /* i_nCmdShow */)
{
    g_hInst = i_hInstance;

    // Convert command line to UTF-8
    std::string cmdLine = yamy::platform::wstring_to_utf8(i_lpszCmdLine);

    // Check for debug flags EARLY
    bool debugMode = (cmdLine.find("--debug") != std::string::npos ||
                      cmdLine.find("-d") != std::string::npos ||
                      GetEnvironmentVariableA("YAMY_DEBUG", nullptr, 0) > 0);

    bool showVersion = (cmdLine.find("--version") != std::string::npos ||
                        cmdLine.find("-v") != std::string::npos);

    bool showHelp = (cmdLine.find("--help") != std::string::npos ||
                     cmdLine.find("-h") != std::string::npos);

    // Always enable file logging
    yamy::debug::DebugConsole::EnableFileLogging();
    yamy::debug::DebugConsole::LogInfo("YAMY starting...");
    yamy::debug::DebugConsole::LogInfo("Command line: " + cmdLine);

    // Enable console in debug mode
    if (debugMode) {
        yamy::debug::DebugConsole::AllocateConsole();
        yamy::debug::DebugConsole::LogInfo("Debug mode enabled");
    }

    if (showVersion) {
        yamy::debug::DebugConsole::LogInfo("YAMY version 1.0.1");
        MessageBoxA(NULL, "YAMY version 1.0.1\nKeyboard remapper for Windows",
                    "YAMY Version", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    if (showHelp) {
        std::string helpText =
            "YAMY - Keyboard Remapper\n\n"
            "Usage: yamy.exe [options]\n\n"
            "Options:\n"
            "  --debug, -d       Enable debug console and verbose logging\n"
            "  --version, -v     Show version information\n"
            "  --help, -h        Show this help message\n\n"
            "Log file location: " + yamy::debug::DebugConsole::GetLogPath();
        yamy::debug::DebugConsole::LogInfo(helpText);
        MessageBoxA(NULL, helpText.c_str(), "YAMY Help", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    if (_tsetlocale(LC_ALL, _T("")) == nullptr) {
        yamy::debug::DebugConsole::LogWarning("Failed to set locale");
    }

    yamy::debug::DebugConsole::LogInfo("Initializing common controls...");
    InitCommonControls();

    yamy::debug::DebugConsole::LogInfo("Initializing OLE...");
    if (FAILED(OleInitialize(nullptr))) {
        yamy::debug::DebugConsole::CriticalError("Failed to initialize OLE");
        return 0;
    }

    // Rest of application uses UTF-8 internally
    yamy::debug::DebugConsole::LogInfo("Entering main application...");
    int result = appMain(cmdLine);

    yamy::debug::DebugConsole::LogInfo("Application exited with code: " + std::to_string(result));
    yamy::debug::DebugConsole::Shutdown();

    OleUninitialize();
    return result;
}

int appMain(const std::string& /*cmdLine*/) {
    yamy::debug::DebugConsole::LogInfo("Checking for existing instance...");

    // Mutex
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"Ctl_Mayu_Mutex");
    DWORD lastError = GetLastError();

    if (lastError == ERROR_ALREADY_EXISTS) {
        yamy::debug::DebugConsole::LogWarning("YAMY is already running. Exiting.");
        MessageBoxW(nullptr, L"YAMY is already running.\n\nCheck the system tray for the YAMY icon.",
                    L"YAMY Already Running", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    yamy::debug::DebugConsole::LogInfo("No existing instance found. Starting YAMY...");

    int result = 0;
    try {
        yamy::debug::DebugConsole::LogInfo("Creating Mayu object...");
        Mayu mayu(hMutex); // Pass mutex ownership to Mayu

        yamy::debug::DebugConsole::LogInfo("Mayu object created successfully");
        yamy::debug::DebugConsole::LogInfo("Entering message loop...");

        result = (int)mayu.messageLoop();

        yamy::debug::DebugConsole::LogInfo("Message loop exited");
    }
    catch (const std::exception& e) {
        std::string errorMsg = "Exception caught: " + std::string(e.what());
        yamy::debug::DebugConsole::CriticalError(errorMsg);
    }
    catch (...) {
        yamy::debug::DebugConsole::CriticalError("Unknown exception caught!");
    }

    return result;
}
