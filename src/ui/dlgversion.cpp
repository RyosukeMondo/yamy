//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dlgversion.cpp


#include "misc.h"

#include "mayu.h"
#include "mayurc.h"
#include "windowstool.h"
#include "compiler_specific_func.h"
#include "layoutmanager.h"

#include <cstdio>
#include <windowsx.h>

#ifndef VERSION
#define VERSION "0.04"
#endif
#ifndef LOGNAME
#define LOGNAME "unknown"
#endif
#ifndef COMPUTERNAME
#define COMPUTERNAME "unknown"
#endif


///
class DlgVersion : public LayoutManager
{
    HWND m_hwnd;        ///

public:
    ///
    DlgVersion(HWND i_hwnd)
            : LayoutManager(i_hwnd),
            m_hwnd(i_hwnd) {
    }

    ///
    virtual ~DlgVersion() {}

    /// WM_INITDIALOG
    BOOL wmInitDialog(HWND /* i_focus */, LPARAM i_lParam) {
        setSmallIcon(m_hwnd, IDI_ICON_mayu);
        setBigIcon(m_hwnd, IDI_ICON_mayu);

        _TCHAR modulebuf[1024];
        CHECK_TRUE( GetModuleFileName(g_hInst, modulebuf,
                                      NUMBER_OF(modulebuf)) );

        std::string modulebuf_str = yamy::platform::wstring_to_utf8(modulebuf);
        std::string version_fmt = yamy::platform::wstring_to_utf8(loadString(IDS_version));
        std::string homepage = yamy::platform::wstring_to_utf8(loadString(IDS_homepage));
        std::string built_by = std::string(LOGNAME) + "@" + yamy::platform::wstring_to_utf8(toLower(_T(COMPUTERNAME)));
        std::string compiler_ver = yamy::platform::wstring_to_utf8(getCompilerVersionString());

        std::string version = VERSION;
#ifndef NDEBUG
        version += " (DEBUG)";
#endif // !NDEBUG
#ifdef _UNICODE
        version += " (UNICODE)";
#endif // !_UNICODE

        char buf[1024];
        // Note: loadString(IDS_version) format specifiers might be %s which expects char* in std::string context
        // but original code used _sntprintf (TCHAR).
        // If IDS_version contains %s, and we use snprintf, it should be fine.
        // Assuming IDS_version is something like "Version %s\nHomepage: %s\nBuilt by: %s\nDate: %s\nCompiler: %s\nModule: %s"
        snprintf(buf, sizeof(buf), version_fmt.c_str(),
                   version.c_str(),
                   homepage.c_str(),
                   built_by.c_str(),
                   __DATE__ " " __TIME__,
                   compiler_ver.c_str(),
                   modulebuf_str.c_str());


        yamy::windows::setWindowText(GetDlgItem(m_hwnd, IDC_EDIT_builtBy), buf);

        // set layout manager
        typedef LayoutManager LM;

        addItem(GetDlgItem(m_hwnd, IDC_STATIC_mayuIcon),
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_TOP_EDGE,
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_TOP_EDGE);
        addItem(GetDlgItem(m_hwnd, IDC_EDIT_builtBy),
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_TOP_EDGE,
                LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_BOTTOM_EDGE);
        addItem(GetDlgItem(m_hwnd, IDC_BUTTON_download),
                LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE,
                LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE);
        addItem(GetDlgItem(m_hwnd, IDOK),
                LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE,
                LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE);
        restrictSmallestSize();

        return TRUE;
    }

    /// WM_CLOSE
    BOOL wmClose() {
        CHECK_TRUE( EndDialog(m_hwnd, 0) );
        return TRUE;
    }

    /// WM_COMMAND
    BOOL wmCommand(int /* i_notifyCode */, int i_id, HWND /* i_hwndControl */) {
        switch (i_id) {
        case IDOK: {
            CHECK_TRUE( EndDialog(m_hwnd, 0) );
            return TRUE;
        }
        case IDC_BUTTON_download: {
<<<<<<< HEAD
            ShellExecute(nullptr, nullptr, to_tstring(loadString(IDS_homepage)).c_str(),
=======
            ShellExecuteW(nullptr, nullptr, loadString(IDS_homepage).c_str(),
>>>>>>> origin/master
                         nullptr, nullptr, SW_SHOWNORMAL);
            CHECK_TRUE( EndDialog(m_hwnd, 0) );
            return TRUE;
        }
        }
        return FALSE;
    }
};


//
#ifdef MAYU64
INT_PTR CALLBACK dlgVersion_dlgProc(
#else
BOOL CALLBACK dlgVersion_dlgProc(
#endif
    HWND i_hwnd, UINT i_message, WPARAM i_wParam, LPARAM i_lParam)
{
    DlgVersion *wc;
    getUserData(i_hwnd, &wc);
    if (!wc)
        switch (i_message) {
        case WM_INITDIALOG:
            wc = setUserData(i_hwnd, new DlgVersion(i_hwnd));
            return wc->wmInitDialog(reinterpret_cast<HWND>(i_wParam), i_lParam);
        }
    else
        switch (i_message) {
        case WM_COMMAND:
            return wc->wmCommand(HIWORD(i_wParam), LOWORD(i_wParam),
                                 reinterpret_cast<HWND>(i_lParam));
        case WM_CLOSE:
            return wc->wmClose();
        case WM_NCDESTROY:
            delete wc;
            return TRUE;
        default:
            return wc->defaultWMHandler(i_message, i_wParam, i_lParam);
        }
    return FALSE;
}
