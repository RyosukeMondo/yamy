//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// function.cpp


#include "engine.h"
#include "hook.h"
#include "mayu.h"
#include "mayurc.h"
#include "misc.h"
#include "setting_loader.h"
#include "vkeytable.h"
#include "windowstool.h"
#include <algorithm>
#include <process.h>
#include <string>
#include <cstring>
#include "../../platform/windows/utf_conversion.h"

#include "function_data.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TypeTable


template <class T> class TypeTable
{
public:
    T m_type;
    const char *m_name;
};


template <class T> static inline
bool getTypeName(std::string *o_name, T i_type,
                 const TypeTable<T> *i_table, size_t i_n)
{
    for (size_t i = 0; i < i_n; ++ i)
        if (i_table[i].m_type == i_type) {
            *o_name = i_table[i].m_name;
            return true;
        }
    return false;
}

template <class T> static inline
bool getTypeValue(T *o_type, const std::string &i_name,
                  const TypeTable<T> *i_table, size_t i_n)
{
    for (size_t i = 0; i < i_n; ++ i)
        if (_stricmp(i_table[i].m_name, i_name.c_str()) == 0) {
            *o_type = i_table[i].m_type;
            return true;
        }
    return false;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// VKey


// stream output
tostream &operator<<(tostream &i_ost, VKey i_data)
{
    if (i_data & VKey_extended)
        i_ost << "E-";
    if (i_data & VKey_released)
        i_ost << "U-";
    if (i_data & VKey_pressed)
        i_ost << "D-";

    u_int8 code = i_data & ~(VKey_extended | VKey_released | VKey_pressed);
    const VKeyTable *vkt;
    for (vkt = g_vkeyTable; vkt->m_name; ++ vkt)
        if (vkt->m_code == code)
            break;
    if (vkt->m_name)
        i_ost << vkt->m_name;
    else
        i_ost << "0x" << std::hex << code << std::dec;
    return i_ost;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ToWindowType


// ToWindowType table
typedef TypeTable<ToWindowType> TypeTable_ToWindowType;
static const TypeTable_ToWindowType g_toWindowTypeTable[] = {
    { ToWindowType_toOverlappedWindow, "toOverlappedWindow" },
    { ToWindowType_toMainWindow,       "toMainWindow"       },
    { ToWindowType_toItself,           "toItself"           },
    { ToWindowType_toParentWindow,     "toParentWindow"     },
};


// stream output
tostream &operator<<(tostream &i_ost, ToWindowType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_toWindowTypeTable, NUMBER_OF(g_toWindowTypeTable)))
        i_ost << to_tstring(name);
    else
        i_ost << static_cast<int>(i_data);
    return i_ost;
}


// get value of ToWindowType
bool getTypeValue(ToWindowType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name,
                        g_toWindowTypeTable, NUMBER_OF(g_toWindowTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GravityType


// GravityType table
typedef TypeTable<GravityType> TypeTable_GravityType;
static const TypeTable_GravityType g_gravityTypeTable[] = {
    { GravityType_C,  "C"  },
    { GravityType_N,  "N"  },
    { GravityType_E,  "E"  },
    { GravityType_W,  "W"  },
    { GravityType_S,  "S"  },
    { GravityType_NW, "NW" },
    { GravityType_NW, "WN" },
    { GravityType_NE, "NE" },
    { GravityType_NE, "EN" },
    { GravityType_SW, "SW" },
    { GravityType_SW, "WS" },
    { GravityType_SE, "SE" },
    { GravityType_SE, "ES" },
};


// stream output
tostream &operator<<(tostream &i_ost, GravityType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_gravityTypeTable, NUMBER_OF(g_gravityTypeTable)))
        i_ost << to_tstring(name);
    else
        i_ost << "(GravityType internal error)";
    return i_ost;
}


// get value of GravityType
bool getTypeValue(GravityType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name,
                        g_gravityTypeTable, NUMBER_OF(g_gravityTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MouseHookType


// MouseHookType table
typedef TypeTable<MouseHookType> TypeTable_MouseHookType;
static const TypeTable_MouseHookType g_mouseHookTypeTable[] = {
    { MouseHookType_None,  "None"  },
    { MouseHookType_Wheel,  "Wheel"  },
    { MouseHookType_WindowMove,  "WindowMove"  },
};


// stream output
tostream &operator<<(tostream &i_ost, MouseHookType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_mouseHookTypeTable, NUMBER_OF(g_mouseHookTypeTable)))
        i_ost << to_tstring(name);
    else
        i_ost << "(MouseHookType internal error)";
    return i_ost;
}


// get value of MouseHookType
bool getTypeValue(MouseHookType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_mouseHookTypeTable,
                        NUMBER_OF(g_mouseHookTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MayuDialogType


// ModifierLockType table
typedef TypeTable<MayuDialogType> TypeTable_MayuDialogType;
static const TypeTable_MayuDialogType g_mayuDialogTypeTable[] = {
    { MayuDialogType_investigate, "investigate"  },
    { MayuDialogType_log,         "log"          },
};


// stream output
tostream &operator<<(tostream &i_ost, MayuDialogType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_mayuDialogTypeTable, NUMBER_OF(g_mayuDialogTypeTable)))
        i_ost << to_tstring(name);
    else
        i_ost << "(MayuDialogType internal error)";
    return i_ost;
}


// get value of MayuDialogType
bool getTypeValue(MayuDialogType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_mayuDialogTypeTable,
                        NUMBER_OF(g_mayuDialogTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ToggleType


// ToggleType table
typedef TypeTable<ToggleType> TypeTable_ToggleType;
static const TypeTable_ToggleType g_toggleType[] = {
    { ToggleType_toggle, "toggle" },
    { ToggleType_off, "off" },
    { ToggleType_off, "false" },
    { ToggleType_off, "released" },
    { ToggleType_on,  "on"  },
    { ToggleType_on,  "true"  },
    { ToggleType_on,  "pressed"  },
};


// stream output
tostream &operator<<(tostream &i_ost, ToggleType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data, g_toggleType, NUMBER_OF(g_toggleType)))
        i_ost << to_tstring(name);
    else
        i_ost << "(ToggleType internal error)";
    return i_ost;
}


// get value of ToggleType
bool getTypeValue(ToggleType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_toggleType,
                        NUMBER_OF(g_toggleType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ModifierLockType


// ModifierLockType table
typedef TypeTable<ModifierLockType> TypeTable_ModifierLockType;
static const TypeTable_ModifierLockType g_modifierLockTypeTable[] = {
    { ModifierLockType_Lock0, "lock0" },
    { ModifierLockType_Lock1, "lock1" },
    { ModifierLockType_Lock2, "lock2" },
    { ModifierLockType_Lock3, "lock3" },
    { ModifierLockType_Lock4, "lock4" },
    { ModifierLockType_Lock5, "lock5" },
    { ModifierLockType_Lock6, "lock6" },
    { ModifierLockType_Lock7, "lock7" },
    { ModifierLockType_Lock8, "lock8" },
    { ModifierLockType_Lock9, "lock9" },
};


// stream output
tostream &operator<<(tostream &i_ost, ModifierLockType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_modifierLockTypeTable, NUMBER_OF(g_modifierLockTypeTable)))
        i_ost << to_tstring(name);
    else
        i_ost << "(ModifierLockType internal error)";
    return i_ost;
}


// get value of ModifierLockType
bool getTypeValue(ModifierLockType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_modifierLockTypeTable,
                        NUMBER_OF(g_modifierLockTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ShowCommandType


// ShowCommandType table
typedef TypeTable<ShowCommandType> TypeTable_ShowCommandType;
static const TypeTable_ShowCommandType g_showCommandTypeTable[] = {
    { ShowCommandType_hide,            "hide"            },
    { ShowCommandType_maximize,        "maximize"        },
    { ShowCommandType_minimize,        "minimize"        },
    { ShowCommandType_restore,         "restore"         },
    { ShowCommandType_show,            "show"            },
    { ShowCommandType_showDefault,     "showDefault"     },
    { ShowCommandType_showMaximized,   "showMaximized"   },
    { ShowCommandType_showMinimized,   "showMinimized"   },
    { ShowCommandType_showMinNoActive, "showMinNoActive" },
    { ShowCommandType_showNA,          "showNA"          },
    { ShowCommandType_showNoActivate,  "showNoActivate"  },
    { ShowCommandType_showNormal,      "showNormal"      },
};


// stream output
tostream &operator<<(tostream &i_ost, ShowCommandType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_showCommandTypeTable, NUMBER_OF(g_showCommandTypeTable)))
        i_ost << to_tstring(name);
    else
        i_ost << "(ShowCommandType internal error)";
    return i_ost;
}


// get value of ShowCommandType
bool getTypeValue(ShowCommandType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_showCommandTypeTable,
                        NUMBER_OF(g_showCommandTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TargetWindowType


// ModifierLockType table
typedef TypeTable<TargetWindowType> TypeTable_TargetWindowType;
static const TypeTable_TargetWindowType g_targetWindowType[] = {
    { TargetWindowType_overlapped, "overlapped" },
    { TargetWindowType_mdi,        "mdi"        },
};


// stream output
tostream &operator<<(tostream &i_ost, TargetWindowType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_targetWindowType, NUMBER_OF(g_targetWindowType)))
        i_ost << to_tstring(name);
    else
        i_ost << "(TargetWindowType internal error)";
    return i_ost;
}


// get value of TargetWindowType
bool getTypeValue(TargetWindowType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_targetWindowType,
                        NUMBER_OF(g_targetWindowType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// BooleanType


// BooleanType table
typedef TypeTable<BooleanType> TypeTable_BooleanType;
static const TypeTable_BooleanType g_booleanType[] = {
    { BooleanType_false, "false" },
    { BooleanType_true,  "true"  },
};


// stream output
tostream &operator<<(tostream &i_ost, BooleanType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data, g_booleanType, NUMBER_OF(g_booleanType)))
        i_ost << to_tstring(name);
    else
        i_ost << "(BooleanType internal error)";
    return i_ost;
}


// get value of BooleanType
bool getTypeValue(BooleanType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_booleanType,
                        NUMBER_OF(g_booleanType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LogicalOperatorType


// LogicalOperatorType table
typedef TypeTable<LogicalOperatorType> TypeTable_LogicalOperatorType;
static const TypeTable_LogicalOperatorType g_logicalOperatorType[] = {
    { LogicalOperatorType_or, "||" },
    { LogicalOperatorType_and,  "&&"  },
};


// stream output
tostream &operator<<(tostream &i_ost, LogicalOperatorType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data, g_logicalOperatorType,
                    NUMBER_OF(g_logicalOperatorType)))
        i_ost << to_tstring(name);
    else
        i_ost << "(LogicalOperatorType internal error)";
    return i_ost;
}


// get value of LogicalOperatorType
bool getTypeValue(LogicalOperatorType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_logicalOperatorType,
                        NUMBER_OF(g_logicalOperatorType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// WindowMonitorFromType


// WindowMonitorFromType table
typedef TypeTable<WindowMonitorFromType> TypeTable_WindowMonitorFromType;
static const TypeTable_WindowMonitorFromType g_windowMonitorFromType[] = {
    { WindowMonitorFromType_primary, "primary" },
    { WindowMonitorFromType_current, "current" },
};


// stream output
tostream &operator<<(tostream &i_ost, WindowMonitorFromType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data, g_windowMonitorFromType,
                    NUMBER_OF(g_windowMonitorFromType)))
        i_ost << to_tstring(name);
    else
        i_ost << "(WindowMonitorFromType internal error)";
    return i_ost;
}


// get value of WindowMonitorFromType
bool getTypeValue(WindowMonitorFromType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_windowMonitorFromType,
                        NUMBER_OF(g_windowMonitorFromType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// std::list<tstringq>


/// stream output
tostream &operator<<(tostream &i_ost, const std::list<std::string> &i_data)
{
    for (std::list<std::string>::const_iterator
            i = i_data.begin(); i != i_data.end(); ++ i) {
        i_ost << to_tstring(*i) << _T(", ");
    }
    return i_ost;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// FunctionData


//
FunctionData::~FunctionData()
{
}


// stream output
tostream &operator<<(tostream &i_ost, const FunctionData *i_data)
{
    return i_data->output(i_ost);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// misc. functions


//
bool Engine::getSuitableWindow(FunctionParam *i_param, HWND *o_hwnd)
{
    if (!i_param->m_isPressed)
        return false;
    *o_hwnd = getToplevelWindow(i_param->m_hwnd, nullptr);
    if (!*o_hwnd)
        return false;
    return true;
}

//
bool Engine::getSuitableMdiWindow(yamy::platform::IWindowSystem *ws, FunctionParam *i_param, HWND *o_hwnd,
                          TargetWindowType *io_twt,
                          RECT *o_rcWindow /*= nullptr*/, RECT *o_rcParent /*= nullptr*/)
{
    if (!i_param->m_isPressed)
        return false;
    bool isMdi = *io_twt == TargetWindowType_mdi;
    *o_hwnd = getToplevelWindow(i_param->m_hwnd, &isMdi);
    *io_twt = isMdi ? TargetWindowType_mdi : TargetWindowType_overlapped;
    if (!*o_hwnd)
        return false;
    switch (*io_twt) {
    case TargetWindowType_overlapped:
        if (o_rcWindow) {
            yamy::platform::Rect wr;
            if (ws->getWindowRect((yamy::platform::WindowHandle)*o_hwnd, &wr)) {
                o_rcWindow->left = wr.left;
                o_rcWindow->top = wr.top;
                o_rcWindow->right = wr.right;
                o_rcWindow->bottom = wr.bottom;
            }
        }
        if (o_rcParent) {
            HMONITOR hm = monitorFromWindow(i_param->m_hwnd,
                                            MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi;
            mi.cbSize = sizeof(mi);
            getMonitorInfo(hm, &mi);
            *o_rcParent = mi.rcWork;
        }
        break;
    case TargetWindowType_mdi:
        if (o_rcWindow) {
            yamy::platform::Rect wr;
            if (ws->getChildWindowRect((yamy::platform::WindowHandle)*o_hwnd, &wr)) {
                o_rcWindow->left = wr.left;
                o_rcWindow->top = wr.top;
                o_rcWindow->right = wr.right;
                o_rcWindow->bottom = wr.bottom;
            }
        }
        if (o_rcParent) {
            yamy::platform::Rect wr;
            if (ws->getClientRect(ws->getParent((yamy::platform::WindowHandle)*o_hwnd), &wr)) {
                o_rcParent->left = wr.left;
                o_rcParent->top = wr.top;
                o_rcParent->right = wr.right;
                o_rcParent->bottom = wr.bottom;
            }
        }
        break;
    }
    return true;
}


// EmacsEditKillLineFunc.
// clear the contents of the clopboard
// at that time, confirm if it is the result of the previous kill-line
void Engine::EmacsEditKillLine::func()
{
    if (!m_buf.empty()) {
        HGLOBAL g;
        const _TCHAR *text = clipboardGetText(&g);

        std::string textUtf8;
        if (text) {
             textUtf8 = yamy::platform::wstring_to_utf8(text);
        }

        if (text == nullptr || m_buf != textUtf8)
            reset();
        clipboardClose(g);
    }
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        CloseClipboard();
    }
}


/** if the text of the clipboard is
@doc
<pre>
1: EDIT Control (at EOL C-K): ""            =&gt; buf + "\r\n", Delete
0: EDIT Control (other  C-K): "(.+)"        =&gt; buf + "\1"
0: IE FORM TEXTAREA (at EOL C-K): "\r\n"    =&gt; buf + "\r\n"
2: IE FORM TEXTAREA (other C-K): "(.+)\r\n" =&gt; buf + "\1", Return Left
^retval
</pre>
*/
HGLOBAL Engine::EmacsEditKillLine::makeNewKillLineBuf(
    const char *i_data, int *o_retval)
{
    std::wstring bufW = yamy::platform::utf8_to_wstring(m_buf);
    std::wstring dataW = yamy::platform::utf8_to_wstring(i_data);
    const wchar_t* i_data_w = dataW.c_str();

    size_t len = bufW.size();
    size_t i_data_len = dataW.length();
    len += i_data_len + 3;

    HGLOBAL hdata = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                                len * sizeof(_TCHAR));
    if (!hdata)
        return nullptr;
    _TCHAR *dataNew = reinterpret_cast<_TCHAR *>(GlobalLock(hdata));
    *dataNew = _T('\0');
    if (!bufW.empty())
        _tcscpy(dataNew, bufW.c_str());

    if (3 <= i_data_len &&
            i_data_w[i_data_len - 2] == _T('\r') && i_data_w[i_data_len - 1] == _T('\n')) {
        _tcscat(dataNew, i_data_w);
        size_t newLen = _tcslen(dataNew);
        dataNew[newLen - 2] = _T('\0'); // chomp
        *o_retval = 2;
    } else if (i_data_len == 0) {
        _tcscat(dataNew, _T("\r\n"));
        *o_retval = 1;
    } else {
        _tcscat(dataNew, i_data_w);
        *o_retval = 0;
    }

    m_buf = yamy::platform::wstring_to_utf8(dataNew);

    GlobalUnlock(hdata);
    return hdata;
}


// EmacsEditKillLinePred
int Engine::EmacsEditKillLine::pred()
{
    HGLOBAL g;
    const _TCHAR *text = clipboardGetText(&g);
    int retval;
    std::string textUtf8;
    if (text) textUtf8 = yamy::platform::wstring_to_utf8(text);
    HGLOBAL hdata = makeNewKillLineBuf(textUtf8.c_str(), &retval);
    clipboardClose(g, hdata);
    return retval;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// functions


// send a default key to Windows
// funcDefault moved to src/core/commands/cmd_default.cpp

// use a corresponding key of a parent keymap
// funcKeymapParent moved to src/core/commands/cmd_keymap_parent.cpp

// use a corresponding key of a current window
// funcKeymapWindow moved to src/core/commands/cmd_keymap_window.cpp

// use a corresponding key of the previous prefixed keymap
// funcKeymapPrevPrefix moved to src/core/commands/cmd_keymap_prev_prefix.cpp

// use a corresponding key of an other window class, or use a default key
// funcOtherWindowClass moved to src/core/commands/cmd_other_window_class.cpp

// funcPrefix moved to src/core/commands/cmd_prefix.cpp

// funcKeymap moved to src/core/commands/cmd_keymap.cpp
// funcSync moved to src/core/commands/cmd_sync.cpp
// funcToggle moved to src/core/commands/cmd_toggle.cpp
// funcEditNextModifier moved to src/core/commands/cmd_edit_next_modifier.cpp
// funcVariable moved to src/core/commands/cmd_variable.cpp



// ShellExecute
// funcShellExecute moved to src/core/commands/cmd_shell_execute.cpp

#include "../commands/cmd_shell_execute.h"
// shell execute
void Engine::shellExecute()
{
    Command_ShellExecute::executeOnMainThread(this);
}

// SetForegroundWindow
// funcSetForegroundWindow moved to src/core/commands/cmd_set_foreground_window.cpp

// load setting
// funcLoadSetting moved to src/core/commands/cmd_load_setting.cpp

// virtual key


// wait


// investigate WM_COMMAND, WM_SYSCOMMAND
// funcInvestigateCommand moved to src/core/commands/cmd_investigate_command.cpp

// show mayu dialog box
// funcMayuDialog moved to src/core/commands/cmd_mayu_dialog.cpp

// describe bindings
// funcDescribeBindings moved to src/core/commands/cmd_describe_bindings.cpp

// show help message
// funcHelpMessage moved to src/core/commands/cmd_help_message.cpp

// show variable
// funcHelpVariable moved to src/core/commands/cmd_help_variable.cpp

// raise window
// funcWindowRaise moved to src/core/commands/cmd_window_raise.cpp

// lower window
// funcWindowLower moved to src/core/commands/cmd_window_lower.cpp

// minimize window
// funcWindowMinimize moved to src/core/commands/cmd_window_minimize.cpp

// maximize window
// funcWindowMaximize moved to src/core/commands/cmd_window_maximize.cpp

// maximize horizontally or virtically
// funcWindowHVMaximize moved to src/core/commands/cmd_window_hv_maximize.cpp

// close window
// funcWindowClose moved to src/core/commands/cmd_window_close.cpp

// toggle top-most flag of the window
// funcWindowToggleTopMost moved to src/core/commands/cmd_window_toggle_top_most.cpp

// identify the window
// funcWindowIdentify moved to src/core/commands/cmd_window_identify.cpp

// set alpha blending parameter to the window
// funcWindowSetAlpha moved to src/core/commands/cmd_window_set_alpha.cpp


// redraw the window
// funcWindowRedraw moved to src/core/commands/cmd_window_redraw.cpp

// move window to ...
// funcWindowMoveTo moved to src/core/commands/cmd_window_move_to.cpp

// move window
// funcWindowMove moved to src/core/commands/cmd_window_move.cpp

// maximize window horizontally
// funcWindowHMaximize moved to src/core/commands/cmd_window_h_maximize.cpp

// maximize window vertically
// funcWindowVMaximize moved to src/core/commands/cmd_window_v_maximize.cpp


// move window visibly
// funcWindowMoveVisibly moved to src/core/commands/cmd_window_move_visibly.cpp


struct EnumDisplayMonitorsForWindowMonitorToParam {
    std::vector<HMONITOR> m_monitors;
    std::vector<MONITORINFO> m_monitorinfos;
    int m_primaryMonitorIdx;
    int m_currentMonitorIdx;

    HMONITOR m_hmon;

public:
    EnumDisplayMonitorsForWindowMonitorToParam(HMONITOR i_hmon)
            : m_primaryMonitorIdx(-1), m_currentMonitorIdx(-1), m_hmon(i_hmon) {
    }
};

static BOOL CALLBACK enumDisplayMonitorsForWindowMonitorTo(
    HMONITOR i_hmon, HDC i_hdc, LPRECT i_rcMonitor, LPARAM i_data)
{
    EnumDisplayMonitorsForWindowMonitorToParam &ep =
        *reinterpret_cast<EnumDisplayMonitorsForWindowMonitorToParam *>(i_data);

    ep.m_monitors.push_back(i_hmon);

    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    getMonitorInfo(i_hmon, &mi);
    ep.m_monitorinfos.push_back(mi);

    if (mi.dwFlags & MONITORINFOF_PRIMARY)
        ep.m_primaryMonitorIdx = (int)(ep.m_monitors.size() - 1);
    if (i_hmon == ep.m_hmon)
        ep.m_currentMonitorIdx = (int)(ep.m_monitors.size() - 1);

    return TRUE;
}

/// move window to other monitor
// funcWindowMonitorTo moved to src/core/commands/cmd_window_monitor_to.cpp

/// move window to other monitor
// funcWindowMonitor moved to src/core/commands/cmd_window_monitor.cpp


//
// funcWindowClingToLeft moved to src/core/commands/cmd_window_cling_to_left.cpp

//
// funcWindowClingToRight moved to src/core/commands/cmd_window_cling_to_right.cpp

//
// funcWindowClingToTop moved to src/core/commands/cmd_window_cling_to_top.cpp

//
// funcWindowClingToBottom moved to src/core/commands/cmd_window_cling_to_bottom.cpp

// resize window to
// funcWindowResizeTo moved to src/core/commands/cmd_window_resize_to.cpp

// move the mouse cursor
// funcMouseMove moved to src/core/commands/cmd_mouse_move.cpp

// send a mouse-wheel-message to Windows
// funcMouseWheel moved to src/core/commands/cmd_mouse_wheel.cpp

// funcClipboardChangeCase moved to src/core/commands/cmd_clipboard_change_case.cpp

// convert the contents of the Clipboard to upper case
// funcClipboardUpcaseWord moved to src/core/commands/cmd_clipboard_upcase_word.cpp

// convert the contents of the Clipboard to lower case
// funcClipboardDowncaseWord moved to src/core/commands/cmd_clipboard_downcase_word.cpp

// set the contents of the Clipboard to the string
// funcClipboardCopy moved to src/core/commands/cmd_clipboard_copy.cpp

//
// funcEmacsEditKillLinePred moved to src/core/commands/cmd_emacs_edit_kill_line_pred.cpp

//
// funcEmacsEditKillLineFunc moved to src/core/commands/cmd_emacs_edit_kill_line_func.cpp

// clear log
// funcLogClear moved to src/core/commands/cmd_log_clear.cpp

// recenter
// funcRecenter moved to src/core/commands/cmd_recenter.cpp

// set IME open status
// funcSetImeStatus moved to src/core/commands/cmd_set_ime_status.cpp

// set IME open status
// funcSetImeString moved to src/core/commands/cmd_set_ime_string.cpp

// Direct SSTP
// funcDirectSSTP moved to src/core/commands/cmd_direct_sstp.cpp

// PlugIn
// funcPlugIn moved to src/core/commands/cmd_plugin.cpp

// funcMouseHook moved to src/core/commands/cmd_mouse_hook.cpp

// cancel prefix
// funcCancelPrefix moved to src/core/commands/cmd_cancel_prefix.cpp


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

