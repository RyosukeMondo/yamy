//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// target.cpp


#include "misc.h"

#include "mayurc.h"
#include "target.h"
#include "windowstool.h"
#include "../platform/types.h"


///
class Target
{
    yamy::platform::WindowHandle m_hwnd;                    ///
    yamy::platform::WindowHandle m_preHwnd;                ///
    HICON m_hCursor;                ///

    ///
    static void invertFrame(yamy::platform::WindowHandle i_hwnd) {
        HWND hwnd = static_cast<HWND>(i_hwnd);
        HDC hdc = GetWindowDC(hwnd);
        ASSERT(hdc);
        int rop2 = SetROP2(hdc, R2_XORPEN);
        if (rop2) {
            RECT rc;
            CHECK_TRUE( GetWindowRect(hwnd, &rc) );
            int width = rcWidth(&rc);
            int height = rcHeight(&rc);

            HANDLE hpen = SelectObject(hdc, GetStockObject(WHITE_PEN));
            HANDLE hbr  = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            CHECK_TRUE( Rectangle(hdc, 0, 0, width    , height    ) );
            CHECK_TRUE( Rectangle(hdc, 1, 1, width - 1, height - 1) );
            CHECK_TRUE( Rectangle(hdc, 2, 2, width - 2, height - 2) );
            SelectObject(hdc, hpen);
            SelectObject(hdc, hbr);
            // no need to DeleteObject StockObject
            SetROP2(hdc, rop2);
        }
        CHECK_TRUE( ReleaseDC(hwnd, hdc) );
    }

    ///
    Target(yamy::platform::WindowHandle i_hwnd)
            : m_hwnd(i_hwnd),
            m_preHwnd(nullptr),
            m_hCursor(nullptr) {
    }

    /// WM_CREATE
    int wmCreate(CREATESTRUCT * /* i_cs */) {
        CHECK_TRUE( m_hCursor =
                        LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_CURSOR_target)) );
        return 0;
    }

    /// WM_PAINT
    int wmPaint() {
        PAINTSTRUCT ps;
        HWND hwnd = static_cast<HWND>(m_hwnd);
        HDC hdc = BeginPaint(hwnd, &ps);
        ASSERT(hdc);

        if (GetCapture() != hwnd) {
            RECT rc;
            CHECK_TRUE( GetClientRect(hwnd, &rc) );
            CHECK_TRUE(
                DrawIcon(hdc, (rcWidth(&rc) - GetSystemMetrics(SM_CXICON)) / 2,
                         (rcHeight(&rc) - GetSystemMetrics(SM_CYICON)) / 2,
                         m_hCursor) );
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    ///
    struct PointWindow {
        yamy::platform::Point m_p;                    ///
        yamy::platform::WindowHandle m_hwnd;                ///
        yamy::platform::Rect m_rc;                    ///
    };

    ///
    static int CALLBACK childWindowFromPoint(yamy::platform::WindowHandle i_hwnd, intptr_t i_lParam) {
        HWND hwnd = static_cast<HWND>(i_hwnd);
        if (IsWindowVisible(hwnd)) {
            PointWindow &pw = *(PointWindow *)i_lParam;
            RECT rc;
            CHECK_TRUE( GetWindowRect(hwnd, &rc) );
            POINT pt = { pw.m_p.x, pw.m_p.y };
            if (PtInRect(&rc, pt)) {
                RECT rcPw = { pw.m_rc.left, pw.m_rc.top, pw.m_rc.right, pw.m_rc.bottom };
                if (isRectInRect(&rc, &rcPw)) {
                    pw.m_hwnd = i_hwnd;
                    pw.m_rc.left = rc.left; pw.m_rc.top = rc.top; pw.m_rc.right = rc.right; pw.m_rc.bottom = rc.bottom;
                }
            }
        }
        return true;
    }

    ///
    static int CALLBACK windowFromPoint(yamy::platform::WindowHandle i_hwnd, intptr_t i_lParam) {
        HWND hwnd = static_cast<HWND>(i_hwnd);
        if (IsWindowVisible(hwnd)) {
            PointWindow &pw = *(PointWindow *)i_lParam;
            RECT rc;
            CHECK_TRUE( GetWindowRect(hwnd, &rc) );
            POINT pt = { pw.m_p.x, pw.m_p.y };
            if (PtInRect(&rc, pt)) {
                pw.m_hwnd = i_hwnd;
                pw.m_rc.left = rc.left; pw.m_rc.top = rc.top; pw.m_rc.right = rc.right; pw.m_rc.bottom = rc.bottom;
                return false;
            }
        }
        return true;
    }

    /// WM_MOUSEMOVE
    int wmMouseMove(uint16_t /* i_keys */, int /* i_x */, int /* i_y */) {
        HWND hwnd = static_cast<HWND>(m_hwnd);
        if (GetCapture() == hwnd) {
            PointWindow pw;
            POINT pt;
            CHECK_TRUE( GetCursorPos(&pt) );
            pw.m_p.x = pt.x; pw.m_p.y = pt.y;
            pw.m_hwnd = 0;
            RECT rc;
            CHECK_TRUE( GetWindowRect(GetDesktopWindow(), &rc) );
            pw.m_rc.left = rc.left; pw.m_rc.top = rc.top; pw.m_rc.right = rc.right; pw.m_rc.bottom = rc.bottom;

            EnumWindows(reinterpret_cast<WNDENUMPROC>(windowFromPoint), (LPARAM)&pw);
            while (1) {
                yamy::platform::WindowHandle hwndParent = pw.m_hwnd;
                if (!EnumChildWindows(static_cast<HWND>(pw.m_hwnd), reinterpret_cast<WNDENUMPROC>(childWindowFromPoint), (LPARAM)&pw))
                    break;
                if (hwndParent == pw.m_hwnd)
                    break;
            }
            if (pw.m_hwnd != m_preHwnd) {
                if (m_preHwnd)
                    invertFrame(m_preHwnd);
                m_preHwnd = pw.m_hwnd;
                invertFrame(m_preHwnd);
                SendMessage(GetParent(hwnd), WM_APP_targetNotify, 0,
                            (LPARAM)m_preHwnd);
            }
            SetCursor(m_hCursor);
        }
        return 0;
    }

    /// WM_LBUTTONDOWN
    int wmLButtonDown(uint16_t /* i_keys */, int /* i_x */, int /* i_y */) {
        HWND hwnd = static_cast<HWND>(m_hwnd);
        SetCapture(hwnd);
        SetCursor(m_hCursor);
        CHECK_TRUE( InvalidateRect(hwnd, nullptr, TRUE) );
        CHECK_TRUE( UpdateWindow(hwnd) );
        return 0;
    }

    /// WM_LBUTTONUP
    int wmLButtonUp(uint16_t /* i_keys */, int /* i_x */, int /* i_y */) {
        HWND hwnd = static_cast<HWND>(m_hwnd);
        if (m_preHwnd)
            invertFrame(m_preHwnd);
        m_preHwnd = nullptr;
        ReleaseCapture();
        CHECK_TRUE( InvalidateRect(hwnd, nullptr, TRUE) );
        CHECK_TRUE( UpdateWindow(hwnd) );
        return 0;
    }

public:
    ///
    static LRESULT CALLBACK WndProc(HWND i_hwnd, UINT i_message,
                                    WPARAM i_wParam, LPARAM i_lParam) {
        Target *wc;
        getUserData(i_hwnd, &wc);
        if (!wc)
            switch (i_message) {
            case WM_CREATE:
                wc = setUserData(i_hwnd, new Target(static_cast<yamy::platform::WindowHandle>(i_hwnd)));
                return wc->wmCreate((CREATESTRUCT *)i_lParam);
            }
        else
            switch (i_message) {
            case WM_PAINT:
                return wc->wmPaint();
            case WM_LBUTTONDOWN:
                return wc->wmLButtonDown((uint16_t)i_wParam, (short)LOWORD(i_lParam),
                                         (short)HIWORD(i_lParam));
            case WM_LBUTTONUP:
                return wc->wmLButtonUp((uint16_t)i_wParam, (short)LOWORD(i_lParam),
                                       (short)HIWORD(i_lParam));
            case WM_MOUSEMOVE:
                return wc->wmMouseMove((uint16_t)i_wParam, (short)LOWORD(i_lParam),
                                       (short)HIWORD(i_lParam));
            case WM_NCDESTROY:
                delete wc;
                return 0;
            }
        return DefWindowProc(i_hwnd, i_message, i_wParam, i_lParam);
    }
};


//
uint16_t Register_target()
{
    WNDCLASS wc;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = Target::WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = g_hInst;
    wc.hIcon         = nullptr;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = "mayuTarget";
    return RegisterClass(&wc);
}
