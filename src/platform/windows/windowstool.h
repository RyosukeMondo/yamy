#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// windowstool.h


#ifndef _WINDOWSTOOL_H
#  define _WINDOWSTOOL_H


#  include "stringtool.h"
#  include <windows.h>
#  include <string>
#  include "utf_conversion.h"
#  include "../../core/platform/types.h"

namespace yamy::windows {

// Wrap SetWindowText to work with UTF-8
inline void setWindowText(HWND hwnd, const std::string& text) {
    SetWindowTextW(hwnd, yamy::platform::utf8_to_wstring(text).c_str());
}

// Wrap GetWindowText to return UTF-8
inline std::string getWindowText(HWND hwnd) {
    int len = GetWindowTextLengthW(hwnd);
    if (len == 0) return std::string();

    std::wstring wide(len + 1, L'\0');
    GetWindowTextW(hwnd, &wide[0], len + 1);
    wide.resize(len); // Remove the null terminator
    return yamy::platform::wstring_to_utf8(wide);
}

// Wrap SetDlgItemText
inline void setDlgItemText(HWND hwnd, int itemId, const std::string& text) {
    SetDlgItemTextW(hwnd, itemId, yamy::platform::utf8_to_wstring(text).c_str());
}

// Wrap GetDlgItemText
inline std::string getDlgItemText(HWND hwnd, int itemId) {
    int len = GetWindowTextLengthW(GetDlgItem(hwnd, itemId));
    if (len == 0) return std::string();

    std::wstring wide(len + 1, L'\0');
    GetDlgItemTextW(hwnd, itemId, &wide[0], len + 1);
    wide.resize(len); // Remove the null terminator
    return yamy::platform::wstring_to_utf8(wide);
}

// Wrap MessageBox
inline int messageBox(HWND hwnd, const std::string& text,
                      const std::string& caption, UINT type) {
    return MessageBoxW(hwnd,
                      yamy::platform::utf8_to_wstring(text).c_str(),
                      yamy::platform::utf8_to_wstring(caption).c_str(),
                      type);
}

} // namespace yamy::windows

/// instance handle of this application
extern HINSTANCE g_hInst;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// resource

/// load resource string
extern std::string loadString(UINT i_id);

// Legacy support: overload for tstring return type isn't possible directly by name.
// We can't overload on return type.
// But we can check where loadString is used. If it's assigned to tstring, we might need manual conversion
// or we assume tstring = std::string if I remove typedef (but I'm not removing it yet).
// For now, I will rename the new one or replace it?
// The instruction said: "Old: tstring loadString(UINT id); New: std::string loadString(UINT id);"
// If I change it, call sites expecting tstring will fail if implicit conversion from std::string to tstring (wstring) doesn't exist.
// std::string to std::wstring is NOT implicit.
// So I will create a `loadStringT` or keep `loadString` returning `tstring` as a wrapper?
// Or I break the build? The instructions say "Update windowstool.cpp functions".
// "Old: tstring loadString(UINT id); New: std::string loadString(UINT id);"
// I will implement `std::string loadString` and if I need to support legacy, I might need another name or force callers to convert.
// Let's check usages of loadString.

/// load small icon resource (it must be deleted by DestroyIcon())
extern HICON loadSmallIcon(UINT i_id);

///load big icon resource (it must be deleted by DestroyIcon())
extern HICON loadBigIcon(UINT i_id);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window

/// resize the window (it does not move the window)
extern bool resizeWindow(HWND i_hwnd, int i_w, int i_h, bool i_doRepaint);

/** get rect of the window in client coordinates.
    @return rect of the window in client coordinates */
extern bool getChildWindowRect(HWND i_hwnd, RECT *o_rc);

/** set small icon to the specified window.
    @return handle of previous icon or nullptr */
extern HICON setSmallIcon(HWND i_hwnd, UINT i_id);

/** set big icon to the specified window.
    @return handle of previous icon or nullptr */
extern HICON setBigIcon(HWND i_hwnd, UINT i_id);

/// remove icon from a window that is set by setSmallIcon
extern void unsetSmallIcon(HWND i_hwnd);

/// remove icon from a window that is set by setBigIcon
extern void unsetBigIcon(HWND i_hwnd);

/// get toplevel (non-child) window
extern yamy::platform::WindowHandle getToplevelWindow(yamy::platform::WindowHandle i_hwnd, bool *io_isMDI);

/// move window asynchronously
extern void asyncMoveWindow(yamy::platform::WindowHandle i_hwnd, int i_x, int i_y);

/// move window asynchronously
extern void asyncMoveWindow(yamy::platform::WindowHandle i_hwnd, int i_x, int i_y, int i_w, int i_h);

/// resize asynchronously
extern void asyncResize(yamy::platform::WindowHandle i_hwnd, int i_w, int i_h);

/// get dll version
extern DWORD getDllVersion(const std::string &i_dllname);
// Legacy
inline DWORD getDllVersion(const tstring &i_dllname) {
    return getDllVersion(to_string(i_dllname));
}
#define PACKVERSION(major, minor) MAKELONG(minor, major)

// workaround of SetForegroundWindow
extern bool setForegroundWindow(yamy::platform::WindowHandle i_hwnd);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dialog

/// get/set GWL_USERDATA
template <class T> inline T getUserData(HWND i_hwnd, T *i_wc)
{
#ifdef MAYU64
    return (*i_wc = reinterpret_cast<T>(GetWindowLongPtr(i_hwnd, GWLP_USERDATA)));
#else
    return (*i_wc = reinterpret_cast<T>(GetWindowLongPtr(i_hwnd, GWLP_USERDATA)));
#endif
}

///
template <class T> inline T setUserData(HWND i_hwnd, T i_wc)
{
#ifdef MAYU64
    SetWindowLongPtr(i_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(i_wc));
#else
    SetWindowLongPtr(i_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(i_wc));
#endif
    return i_wc;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// RECT

///
inline int rcWidth(const RECT *i_rc)
{
    return i_rc->right - i_rc->left;
}

///
inline int rcHeight(const RECT *i_rc)
{
    return i_rc->bottom - i_rc->top;
}

///
inline bool isRectInRect(const RECT *i_rcin, const RECT *i_rcout)
{
    return (i_rcout->left <= i_rcin->left &&
            i_rcin->right <= i_rcout->right &&
            i_rcout->top <= i_rcin->top &&
            i_rcin->bottom <= i_rcout->bottom);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// edit control

/// returns bytes of text
extern size_t editGetTextBytes(HWND i_hwnd);

/// delete a line
extern void editDeleteLine(HWND i_hwnd, size_t i_n);

/// insert text at last
extern void editInsertTextAtLast(HWND i_hwnd, const std::string &i_text,
                                     size_t i_threshold);
// Legacy
inline void editInsertTextAtLast(HWND i_hwnd, const tstring &i_text, size_t i_threshold) {
    editInsertTextAtLast(i_hwnd, to_string(i_text), i_threshold);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Windows2000/XP specific API

/// SetLayeredWindowAttributes API
typedef BOOL (WINAPI *SetLayeredWindowAttributes_t)
(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
extern SetLayeredWindowAttributes_t setLayeredWindowAttributes;

/// MonitorFromWindow API
extern HMONITOR (WINAPI *monitorFromWindow)(HWND hwnd, DWORD dwFlags);

/// GetMonitorInfo API
extern BOOL (WINAPI *getMonitorInfo)(HMONITOR hMonitor, LPMONITORINFO lpmi);

/// EnumDisplayMonitors API
extern BOOL (WINAPI *enumDisplayMonitors)
    (HDC hdc, LPRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// WindowsXP specific API

/// WTSRegisterSessionNotification API
typedef BOOL (WINAPI *WTSRegisterSessionNotification_t)
(HWND hWnd, DWORD dwFlags);
extern WTSRegisterSessionNotification_t wtsRegisterSessionNotification;

/// WTSUnRegisterSessionNotification API
typedef BOOL (WINAPI *WTSUnRegisterSessionNotification_t)(HWND hWnd);
extern WTSUnRegisterSessionNotification_t wtsUnRegisterSessionNotification;

/// WTSGetActiveConsoleSessionId API
typedef DWORD (WINAPI *WTSGetActiveConsoleSessionId_t)(void);
extern WTSGetActiveConsoleSessionId_t wtsGetActiveConsoleSessionId;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Utility

// PathRemoveFileSpec()
std::string pathRemoveFileSpec(const std::string &i_path);
// Legacy
inline tstring pathRemoveFileSpec(const tstring &i_path) {
    return to_tstring(pathRemoveFileSpec(to_string(i_path)));
}

// check Windows version i_major.i_minor or later
BOOL checkWindowsVersion(DWORD i_major, DWORD i_minor);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Clipboard

/// get clipboard text (you must call clipboardClose())
extern const _TCHAR *clipboardGetText(HGLOBAL *o_hdata);

/// close clipboard that opened by clipboardGetText()
extern void clipboardClose(HGLOBAL i_hdata, HGLOBAL i_hdataNew = nullptr);

#endif // _WINDOWSTOOL_H
