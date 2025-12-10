//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dlgsetting.cpp


#include "misc.h"

#include "mayu.h"
#include "mayurc.h"
#include "registry.h"
#include "stringtool.h"
#include "windowstool.h"
#include "setting.h"
#include "dlgeditsetting.h"
#include "layoutmanager.h"

#include <commctrl.h>
#include <windowsx.h>


///
class DlgSetting : public LayoutManager
{
    HWND m_hwndMayuPaths;                ///

    ///
    Registry m_reg;

    typedef DlgEditSettingData Data;        ///

    ///
    void insertItem(int i_index, const Data &i_data) {
        LVITEM item;
        item.mask = LVIF_TEXT;
        item.iItem = i_index;

        std::wstring wName = yamy::platform::utf8_to_wstring(i_data.m_name);
        item.iSubItem = 0;
        item.pszText = const_cast<wchar_t*>(wName.c_str());
        CHECK_TRUE( ListView_InsertItem(m_hwndMayuPaths, &item) != -1 );

        std::wstring wFilename = yamy::platform::utf8_to_wstring(i_data.m_filename);
        ListView_SetItemText(m_hwndMayuPaths, i_index, 1,
                             const_cast<wchar_t*>(wFilename.c_str()));

        std::wstring wSymbols = yamy::platform::utf8_to_wstring(i_data.m_symbols);
        ListView_SetItemText(m_hwndMayuPaths, i_index, 2,
                             const_cast<wchar_t*>(wSymbols.c_str()));
    }

    ///
    void setItem(int i_index, const Data &i_data) {
        std::wstring wName = yamy::platform::utf8_to_wstring(i_data.m_name);
        ListView_SetItemText(m_hwndMayuPaths, i_index, 0,
                             const_cast<wchar_t*>(wName.c_str()));

        std::wstring wFilename = yamy::platform::utf8_to_wstring(i_data.m_filename);
        ListView_SetItemText(m_hwndMayuPaths, i_index, 1,
                             const_cast<wchar_t*>(wFilename.c_str()));

        std::wstring wSymbols = yamy::platform::utf8_to_wstring(i_data.m_symbols);
        ListView_SetItemText(m_hwndMayuPaths, i_index, 2,
                             const_cast<wchar_t*>(wSymbols.c_str()));
    }

    ///
    void getItem(int i_index, Data *o_data) {
        wchar_t buf[GANA_MAX_PATH];
        LVITEMW item;
        item.mask = LVIF_TEXT;
        item.iItem = i_index;
        item.pszText = buf;
        item.cchTextMax = NUMBER_OF(buf);

        item.iSubItem = 0;
        CHECK_TRUE( SendMessage(m_hwndMayuPaths, LVM_GETITEMW, 0, (LPARAM)&item) );
        o_data->m_name = yamy::platform::wstring_to_utf8(buf);

        item.iSubItem = 1;
        CHECK_TRUE( SendMessage(m_hwndMayuPaths, LVM_GETITEMW, 0, (LPARAM)&item) );
        o_data->m_filename = yamy::platform::wstring_to_utf8(buf);

        item.iSubItem = 2;
        CHECK_TRUE( SendMessage(m_hwndMayuPaths, LVM_GETITEMW, 0, (LPARAM)&item) );
        o_data->m_symbols = yamy::platform::wstring_to_utf8(buf);
    }

    ///
    void setSelectedItem(int i_index) {
        ListView_SetItemState(m_hwndMayuPaths, i_index,
                              LVIS_SELECTED, LVIS_SELECTED);
    }

    ///
    int getSelectedItem() {
        if (ListView_GetSelectedCount(m_hwndMayuPaths) == 0)
            return -1;
        for (int i = 0; ; ++ i) {
            if (ListView_GetItemState(m_hwndMayuPaths, i, LVIS_SELECTED))
                return i;
        }
    }

public:
    ///
    DlgSetting(HWND i_hwnd)
            : LayoutManager(i_hwnd),
            m_hwndMayuPaths(nullptr),
            m_reg(MAYU_REGISTRY_ROOT) {
    }

    virtual ~DlgSetting() {}

    /// WM_INITDIALOG
    BOOL wmInitDialog(HWND /* i_focus */, LPARAM /* i_lParam */) {
        setSmallIcon(m_hwnd, IDI_ICON_mayu);
        setBigIcon(m_hwnd, IDI_ICON_mayu);

        CHECK_TRUE( m_hwndMayuPaths = GetDlgItem(m_hwnd, IDC_LIST_mayuPaths) );

        // create list view colmn
        RECT rc;
        GetClientRect(m_hwndMayuPaths, &rc);

        LVCOLUMN lvc;
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = (rc.right - rc.left) / 3;

        tstring str = to_tstring(loadString(IDS_mayuPathName));
        lvc.pszText = const_cast<_TCHAR *>(str.c_str());
        CHECK( 0 ==, ListView_InsertColumn(m_hwndMayuPaths, 0, &lvc) );
        str = to_tstring(loadString(IDS_mayuPath));
        lvc.pszText = const_cast<_TCHAR *>(str.c_str());
        CHECK( 1 ==, ListView_InsertColumn(m_hwndMayuPaths, 1, &lvc) );
        str = to_tstring(loadString(IDS_mayuSymbols));
        lvc.pszText = const_cast<_TCHAR *>(str.c_str());
        CHECK( 2 ==, ListView_InsertColumn(m_hwndMayuPaths, 2, &lvc) );

        Data data;
        insertItem(0, data);                // TODO: why ?

        // set list view
        // tregex split(_T("^([^;]*);([^;]*);(.*)$"));
        // tstringi dot_mayu;
        // Using std::regex and std::string as requested
        std::regex split("^([^;]*);([^;]*);(.*)$");
        std::string dot_mayu;
        int i;
        for (i = 0; i < MAX_MAYU_REGISTRY_ENTRIES; ++ i) {
            _TCHAR buf[100];
            _sntprintf(buf, NUMBER_OF(buf), _T(".mayu%d"), i);
            // Registry::read uses tstring/TCHAR currently (Branch 7 changes it).
            // I should use temporary tstring if Registry is not updated.
            // But I cannot change Registry here easily without breaking other things or doing Branch 7 work.
            // Wait, I can't easily change `m_reg.read` signature.
            // I'll read into tstring and convert.
            tstring t_dot_mayu;
            if (!m_reg.read(buf, &t_dot_mayu))
                break;

            dot_mayu = yamy::platform::wstring_to_utf8(t_dot_mayu);

            std::smatch what;
            if (std::regex_match(dot_mayu, what, split)) {
                data.m_name = what.str(1);
                data.m_filename = what.str(2);
                data.m_symbols = what.str(3);
                insertItem(i, data);
            }
        }

        CHECK_TRUE( ListView_DeleteItem(m_hwndMayuPaths, i) );    // TODO: why ?

        // arrange list view size
        ListView_SetColumnWidth(m_hwndMayuPaths, 0, LVSCW_AUTOSIZE);
        ListView_SetColumnWidth(m_hwndMayuPaths, 1, LVSCW_AUTOSIZE);
        ListView_SetColumnWidth(m_hwndMayuPaths, 2, LVSCW_AUTOSIZE);

        ListView_SetExtendedListViewStyle(m_hwndMayuPaths, LVS_EX_FULLROWSELECT);

        // set selection
        int index;
        m_reg.read(_T(".mayuIndex"), &index, 0);
        setSelectedItem(index);

        // set layout manager
        typedef LayoutManager LM;
        addItem(GetDlgItem(m_hwnd, IDC_STATIC_mayuPaths),
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_TOP_EDGE,
                LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_BOTTOM_EDGE);
        addItem(GetDlgItem(m_hwnd, IDC_LIST_mayuPaths),
                LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_TOP_EDGE,
                LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_BOTTOM_EDGE);
        addItem(GetDlgItem(m_hwnd, IDC_BUTTON_up),
                LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_CENTER,
                LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_CENTER);
        addItem(GetDlgItem(m_hwnd, IDC_BUTTON_down),
                LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_CENTER,
                LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_CENTER);
        addItem(GetDlgItem(m_hwnd, IDC_BUTTON_add),
                LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE,
                LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE);
        addItem(GetDlgItem(m_hwnd, IDC_BUTTON_edit),
                LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE,
                LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE);
        addItem(GetDlgItem(m_hwnd, IDC_BUTTON_delete),
                LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE,
                LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE);
        addItem(GetDlgItem(m_hwnd, IDCANCEL),
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
        EndDialog(m_hwnd, 0);
        return TRUE;
    }

    /// WM_NOTIFY
    BOOL wmNotify(int i_id, NMHDR *i_nmh) {
        switch (i_id) {
        case IDC_LIST_mayuPaths:
            if (i_nmh->code == NM_DBLCLK)
                FORWARD_WM_COMMAND(m_hwnd, IDC_BUTTON_edit, nullptr, 0, SendMessage);
            return TRUE;
        }
        return TRUE;
    }

    /// WM_COMMAND
    BOOL wmCommand(int /* i_notifyCode */, int i_id, HWND /* i_hwndControl */) {
        _TCHAR buf[GANA_MAX_PATH];
        switch (i_id) {
        case IDC_BUTTON_up:
        case IDC_BUTTON_down: {
            int count = ListView_GetItemCount(m_hwndMayuPaths);
            if (count < 2)
                return TRUE;
            int index = getSelectedItem();
            if (index < 0 ||
                    (i_id == IDC_BUTTON_up && index == 0) ||
                    (i_id == IDC_BUTTON_down && index == count - 1))
                return TRUE;

            int target = (i_id == IDC_BUTTON_up) ? index - 1 : index + 1;

            Data dataIndex, dataTarget;
            getItem(index, &dataIndex);
            getItem(target, &dataTarget);
            setItem(index, dataTarget);
            setItem(target, dataIndex);

            setSelectedItem(target);
            return TRUE;
        }

        case IDC_BUTTON_add: {
            Data data;
            int index = getSelectedItem();
            if (0 <= index)
                getItem(index, &data);
            if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_editSetting),
                               m_hwnd, (DLGPROC)dlgEditSetting_dlgProc, (LPARAM)&data))
                if (!data.m_name.empty()) {
                    insertItem(0, data);
                    setSelectedItem(0);
                }
            return TRUE;
        }

        case IDC_BUTTON_delete: {
            int index = getSelectedItem();
            if (0 <= index) {
                CHECK_TRUE( ListView_DeleteItem(m_hwndMayuPaths, index) );
                int count = ListView_GetItemCount(m_hwndMayuPaths);
                if (count == 0)
                    ;
                else if (count == index)
                    setSelectedItem(index - 1);
                else
                    setSelectedItem(index);
            }
            return TRUE;
        }

        case IDC_BUTTON_edit: {
            Data data;
            int index = getSelectedItem();
            if (index < 0)
                return TRUE;
            getItem(index, &data);
            if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_editSetting),
                               m_hwnd, (DLGPROC)dlgEditSetting_dlgProc, (LPARAM)&data)) {
                setItem(index, data);
                setSelectedItem(index);
            }
            return TRUE;
        }

        case IDOK: {
            int count = ListView_GetItemCount(m_hwndMayuPaths);
            int index;
            for (index = 0; index < count; ++ index) {
                _sntprintf(buf, NUMBER_OF(buf), _T(".mayu%d"), index);
                Data data;
                getItem(index, &data);
                // Convert back to tstring for Registry (until Branch 7)
                tstring val = yamy::platform::utf8_to_wstring(data.m_name + ";" +
                            data.m_filename + ";" + data.m_symbols);
                m_reg.write(buf, val);
            }
            for (; ; ++ index) {
                _sntprintf(buf, NUMBER_OF(buf), _T(".mayu%d"), index);
                if (!m_reg.remove(buf))
                    break;
            }
            index = getSelectedItem();
            if (index < 0)
                index = 0;
            m_reg.write(_T(".mayuIndex"), index);
            EndDialog(m_hwnd, 1);
            return TRUE;
        }

        case IDCANCEL: {
            CHECK_TRUE( EndDialog(m_hwnd, 0) );
            return TRUE;
        }
        }
        return FALSE;
    }
};


//
#ifdef MAYU64
INT_PTR CALLBACK dlgSetting_dlgProc(
#else
BOOL CALLBACK dlgSetting_dlgProc(
#endif
    HWND i_hwnd, UINT i_message, WPARAM i_wParam, LPARAM i_lParam)
{
    DlgSetting *wc;
    getUserData(i_hwnd, &wc);
    if (!wc)
        switch (i_message) {
        case WM_INITDIALOG:
            wc = setUserData(i_hwnd, new DlgSetting(i_hwnd));
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
        case WM_NOTIFY:
            return wc->wmNotify(static_cast<int>(i_wParam),
                                reinterpret_cast<NMHDR *>(i_lParam));
        default:
            return wc->defaultWMHandler(i_message, i_wParam, i_lParam);
        }
    return FALSE;
}
