//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// focus.cpp

#ifdef _WIN32

#include "focus.h"
#include "windowstool.h"
#include "../platform/types.h"


///
static LRESULT CALLBACK WndProc(
    HWND i_hwnd, UINT i_message, WPARAM i_wParam, LPARAM i_lParam)
{
    switch (i_message) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
        SendMessage(GetParent(i_hwnd), WM_APP_notifyVKey, i_wParam, i_lParam);
        return 0;
    case WM_CHAR:
    case WM_DEADCHAR:
        return 0;
    case WM_LBUTTONDOWN: {
        SetFocus(i_hwnd);
        return 0;
    }
    case WM_SETFOCUS: {
        RECT rc;
        GetClientRect(i_hwnd, &rc);
        CreateCaret(i_hwnd, (HBITMAP)nullptr, 2,
                    rcHeight(&rc) / 2);
        ShowCaret(i_hwnd);
        SetCaretPos(rcWidth(&rc) / 2, rcHeight(&rc) / 4);
        SendMessage(GetParent(i_hwnd), WM_APP_notifyFocus,
                    true, (intptr_t)i_hwnd);
        return 0;
    }
    case WM_KILLFOCUS: {
        HideCaret(i_hwnd);
        DestroyCaret();
        SendMessage(GetParent(i_hwnd), WM_APP_notifyFocus,
                    false, (intptr_t)i_hwnd);
        return 0;
    }
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS;
    }
    return DefWindowProc(i_hwnd, i_message, i_wParam, i_lParam);
}


uint16_t Register_focus()
{
    WNDCLASS wc;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = g_hInst;
    wc.hIcon         = nullptr;
    wc.hCursor       = LoadCursor(nullptr, IDC_IBEAM);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = L"mayuFocus";  // Wide string literal for UNICODE
    return RegisterClass(&wc);
}

#endif // _WIN32
