//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dlglog.cpp


#include "misc.h"
#include "mayu.h"
#include "mayurc.h"
#include "registry.h"
#include "windowstool.h"
#include "msgstream.h"
#include "layoutmanager.h"
#include "dlglog.h"
#include <windowsx.h>
#include <cstring> // Added for memset

//
///
class DlgLog : public LayoutManager
{
    HWND m_hwndEdit;                ///
    HWND m_hwndTaskTray;                /// tasktray window
    LOGFONT m_lf;                    ///
    HFONT m_hfontOriginal;            ///
    HFONT m_hfont;                ///
    tomsgstream *m_log;                ///
    yamy::platform::IWindowSystem *m_windowSystem;  /// window system abstraction

public:
    ///
    DlgLog(HWND i_hwnd)
            : LayoutManager(i_hwnd),
            m_hwndEdit(GetDlgItem(static_cast<HWND>(m_hwnd), IDC_EDIT_log)),
            m_hwndTaskTray(nullptr),
            m_hfontOriginal(GetWindowFont(static_cast<HWND>(m_hwnd))),
            m_hfont(nullptr),
            m_windowSystem(nullptr) {
    }

    virtual ~DlgLog() {}

    /// WM_INITDIALOG
    BOOL wmInitDialog(HWND /* i_focus */, LPARAM i_lParam) {
        DlgLogData *dld = reinterpret_cast<DlgLogData *>(i_lParam);
        m_log = dld->m_log;
        m_hwndTaskTray = dld->m_hwndTaskTray;
        m_windowSystem = dld->m_windowSystem;

        // set icons
        setSmallIcon(static_cast<HWND>(m_hwnd), IDI_ICON_mayu);
        setBigIcon(static_cast<HWND>(m_hwnd), IDI_ICON_mayu);

        // set font
#ifdef USE_INI
        Registry::read(0, to_string(_T("yamy")), to_string(_T("logFont")), &m_lf,
                       loadString(IDS_logFont));
#else
        Registry::read(HKEY_CURRENT_USER, to_string(_T("Software\\gimy.net\\yamy")), to_string(_T("logFont")), &m_lf,
                       loadString(IDS_logFont));
#endif
        m_hfont = CreateFontIndirect(&m_lf);
        SetWindowFont(m_hwndEdit, m_hfont, false);

        // resize
        RECT rc;
        CHECK_TRUE( GetClientRect(static_cast<HWND>(m_hwnd), &rc) );
        wmSize(0, (short)rc.right, (short)rc.bottom);

        // debug level
        bool isChecked =
            (IsDlgButtonChecked(static_cast<HWND>(m_hwnd), IDC_CHECK_detail) == BST_CHECKED);
        m_log->setDebugLevel(isChecked ? 1 : 0);

        // set layout manager
        typedef LayoutManager LM;
        addItem(GetDlgItem(static_cast<HWND>(m_hwnd), IDOK),
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_BOTTOM_EDGE,
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_BOTTOM_EDGE);
        addItem(GetDlgItem(static_cast<HWND>(m_hwnd), IDC_EDIT_log),
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_TOP_EDGE,
                LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_BOTTOM_EDGE);
        addItem(GetDlgItem(static_cast<HWND>(m_hwnd), IDC_BUTTON_clearLog),
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_BOTTOM_EDGE,
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_BOTTOM_EDGE);
        addItem(GetDlgItem(static_cast<HWND>(m_hwnd), IDC_BUTTON_changeFont),
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_BOTTOM_EDGE,
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_BOTTOM_EDGE);
        addItem(GetDlgItem(static_cast<HWND>(m_hwnd), IDC_CHECK_detail),
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_BOTTOM_EDGE,
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_BOTTOM_EDGE);
        restrictSmallestSize();

        // enlarge window
        GetWindowRect(static_cast<HWND>(m_hwnd), &rc);
        rc.bottom += (rc.bottom - rc.top) * 3;
        MoveWindow(static_cast<HWND>(m_hwnd), rc.left, rc.top,
                   rc.right - rc.left, rc.bottom - rc.top, true);
        return TRUE;
    }

    /// WM_DESTROY
    BOOL wmDestroy() {
        // unset font
        SetWindowFont(m_hwndEdit, m_hfontOriginal, false);
        DeleteObject(m_hfont);

        // unset icons
        unsetBigIcon(static_cast<HWND>(m_hwnd));
        unsetSmallIcon(static_cast<HWND>(m_hwnd));
        return TRUE;
    }

    /// WM_CLOSE
    BOOL wmClose() {
        if (m_windowSystem) {
            m_windowSystem->showWindow(m_hwnd, SW_HIDE);
        } else {
            ShowWindow(static_cast<HWND>(m_hwnd), SW_HIDE);
        }
        return TRUE;
    }

    /// WM_COMMAND
    BOOL wmCommand(int /* i_notifyCode */, int i_id, HWND /* i_hwndControl */) {
        switch (i_id) {
        case IDOK: {
            if (m_windowSystem) {
                m_windowSystem->showWindow(m_hwnd, SW_HIDE);
            } else {
                ShowWindow(static_cast<HWND>(m_hwnd), SW_HIDE);
            }
            return TRUE;
        }

        case IDC_BUTTON_clearLog: {
            Edit_SetSel(m_hwndEdit, 0, Edit_GetTextLength(m_hwndEdit));
            yamy::windows::setWindowText(m_hwndEdit, "");
            SendMessage(m_hwndTaskTray, WM_APP_dlglogNotify,
                        DlgLogNotify_logCleared, 0);
            return TRUE;
        }

        case IDC_BUTTON_changeFont: {
            CHOOSEFONT cf;
            memset(&cf, 0, sizeof(cf));
            cf.lStructSize = sizeof(cf);
            cf.hwndOwner = static_cast<HWND>(m_hwnd);
            cf.lpLogFont = &m_lf;
            cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
            if (ChooseFont(&cf)) {
                HFONT hfontNew = CreateFontIndirect(&m_lf);
                SetWindowFont(static_cast<HWND>(m_hwnd), hfontNew, true);
                DeleteObject(m_hfont);
                m_hfont = hfontNew;
#ifdef USE_INI
                Registry::write(0, to_string(_T("yamy")), to_string(_T("logFont")), m_lf);
#else
                Registry::write(HKEY_CURRENT_USER, to_string(_T("Software\\gimy.net\\yamy")), to_string(_T("logFont")), m_lf);
#endif
            }
            return TRUE;
        }

        case IDC_CHECK_detail: {
            bool isChecked =
                (IsDlgButtonChecked(static_cast<HWND>(m_hwnd), IDC_CHECK_detail) == BST_CHECKED);
            m_log->setDebugLevel(isChecked ? 1 : 0);
            return TRUE;
        }
        }
        return FALSE;
    }
};


//
#ifdef MAYU64
INT_PTR CALLBACK dlgLog_dlgProc(HWND i_hwnd, UINT i_message,
#else
BOOL CALLBACK dlgLog_dlgProc(HWND i_hwnd, UINT i_message,
#endif
                                WPARAM i_wParam, LPARAM i_lParam)
{
    DlgLog *wc;
    getUserData(i_hwnd, &wc);
    if (!wc)
        switch (i_message) {
        case WM_INITDIALOG:
            wc = setUserData(i_hwnd, new DlgLog(i_hwnd));
            return wc->wmInitDialog(reinterpret_cast<HWND>(i_wParam), i_lParam);
        }
    else
        switch (i_message) {
        case WM_COMMAND:
            return wc->wmCommand(HIWORD(i_wParam), LOWORD(i_wParam),
                                 reinterpret_cast<HWND>(i_lParam));
        case WM_CLOSE:
            return wc->wmClose();
        case WM_DESTROY:
            return wc->wmDestroy();
        case WM_NCDESTROY:
            delete wc;
            return TRUE;
        default:
            return wc->defaultWMHandler(i_message, i_wParam, i_lParam);
        }
    return FALSE;
}
