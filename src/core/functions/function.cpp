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

#include "function_data.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TypeTable


template <class T> class TypeTable
{
public:
    T m_type;
    const _TCHAR *m_name;
};


template <class T> static inline
bool getTypeName(tstring *o_name, T i_type,
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
bool getTypeValue(T *o_type, const tstringi &i_name,
                  const TypeTable<T> *i_table, size_t i_n)
{
    for (size_t i = 0; i < i_n; ++ i)
        if (i_table[i].m_name == i_name) {
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
        i_ost << _T("E-");
    if (i_data & VKey_released)
        i_ost << _T("U-");
    if (i_data & VKey_pressed)
        i_ost << _T("D-");

    u_int8 code = i_data & ~(VKey_extended | VKey_released | VKey_pressed);
    const VKeyTable *vkt;
    for (vkt = g_vkeyTable; vkt->m_name; ++ vkt)
        if (vkt->m_code == code)
            break;
    if (vkt->m_name)
        i_ost << vkt->m_name;
    else
        i_ost << _T("0x") << std::hex << code << std::dec;
    return i_ost;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ToWindowType


// ToWindowType table
typedef TypeTable<ToWindowType> TypeTable_ToWindowType;
static const TypeTable_ToWindowType g_toWindowTypeTable[] = {
    { ToWindowType_toOverlappedWindow, _T("toOverlappedWindow") },
    { ToWindowType_toMainWindow,       _T("toMainWindow")       },
    { ToWindowType_toItself,           _T("toItself")           },
    { ToWindowType_toParentWindow,     _T("toParentWindow")     },
};


// stream output
tostream &operator<<(tostream &i_ost, ToWindowType i_data)
{
    tstring name;
    if (getTypeName(&name, i_data,
                    g_toWindowTypeTable, NUMBER_OF(g_toWindowTypeTable)))
        i_ost << name;
    else
        i_ost << static_cast<int>(i_data);
    return i_ost;
}


// get value of ToWindowType
bool getTypeValue(ToWindowType *o_type, const tstring &i_name)
{
    return getTypeValue(o_type, i_name,
                        g_toWindowTypeTable, NUMBER_OF(g_toWindowTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GravityType


// GravityType table
typedef TypeTable<GravityType> TypeTable_GravityType;
static const TypeTable_GravityType g_gravityTypeTable[] = {
    { GravityType_C,  _T("C")  },
    { GravityType_N,  _T("N")  },
    { GravityType_E,  _T("E")  },
    { GravityType_W,  _T("W")  },
    { GravityType_S,  _T("S")  },
    { GravityType_NW, _T("NW") },
    { GravityType_NW, _T("WN") },
    { GravityType_NE, _T("NE") },
    { GravityType_NE, _T("EN") },
    { GravityType_SW, _T("SW") },
    { GravityType_SW, _T("WS") },
    { GravityType_SE, _T("SE") },
    { GravityType_SE, _T("ES") },
};


// stream output
tostream &operator<<(tostream &i_ost, GravityType i_data)
{
    tstring name;
    if (getTypeName(&name, i_data,
                    g_gravityTypeTable, NUMBER_OF(g_gravityTypeTable)))
        i_ost << name;
    else
        i_ost << _T("(GravityType internal error)");
    return i_ost;
}


// get value of GravityType
bool getTypeValue(GravityType *o_type, const tstring &i_name)
{
    return getTypeValue(o_type, i_name,
                        g_gravityTypeTable, NUMBER_OF(g_gravityTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MouseHookType


// MouseHookType table
typedef TypeTable<MouseHookType> TypeTable_MouseHookType;
static const TypeTable_MouseHookType g_mouseHookTypeTable[] = {
    { MouseHookType_None,  _T("None")  },
    { MouseHookType_Wheel,  _T("Wheel")  },
    { MouseHookType_WindowMove,  _T("WindowMove")  },
};


// stream output
tostream &operator<<(tostream &i_ost, MouseHookType i_data)
{
    tstring name;
    if (getTypeName(&name, i_data,
                    g_mouseHookTypeTable, NUMBER_OF(g_mouseHookTypeTable)))
        i_ost << name;
    else
        i_ost << _T("(MouseHookType internal error)");
    return i_ost;
}


// get value of MouseHookType
bool getTypeValue(MouseHookType *o_type, const tstring &i_name)
{
    return getTypeValue(o_type, i_name, g_mouseHookTypeTable,
                        NUMBER_OF(g_mouseHookTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MayuDialogType


// ModifierLockType table
typedef TypeTable<MayuDialogType> TypeTable_MayuDialogType;
static const TypeTable_MayuDialogType g_mayuDialogTypeTable[] = {
    { MayuDialogType_investigate, _T("investigate")  },
    { MayuDialogType_log,         _T("log")          },
};


// stream output
tostream &operator<<(tostream &i_ost, MayuDialogType i_data)
{
    tstring name;
    if (getTypeName(&name, i_data,
                    g_mayuDialogTypeTable, NUMBER_OF(g_mayuDialogTypeTable)))
        i_ost << name;
    else
        i_ost << _T("(MayuDialogType internal error)");
    return i_ost;
}


// get value of MayuDialogType
bool getTypeValue(MayuDialogType *o_type, const tstring &i_name)
{
    return getTypeValue(o_type, i_name, g_mayuDialogTypeTable,
                        NUMBER_OF(g_mayuDialogTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ToggleType


// ToggleType table
typedef TypeTable<ToggleType> TypeTable_ToggleType;
static const TypeTable_ToggleType g_toggleType[] = {
    { ToggleType_toggle, _T("toggle") },
    { ToggleType_off, _T("off") },
    { ToggleType_off, _T("false") },
    { ToggleType_off, _T("released") },
    { ToggleType_on,  _T("on")  },
    { ToggleType_on,  _T("true")  },
    { ToggleType_on,  _T("pressed")  },
};


// stream output
tostream &operator<<(tostream &i_ost, ToggleType i_data)
{
    tstring name;
    if (getTypeName(&name, i_data, g_toggleType, NUMBER_OF(g_toggleType)))
        i_ost << name;
    else
        i_ost << _T("(ToggleType internal error)");
    return i_ost;
}


// get value of ToggleType
bool getTypeValue(ToggleType *o_type, const tstring &i_name)
{
    return getTypeValue(o_type, i_name, g_toggleType,
                        NUMBER_OF(g_toggleType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ModifierLockType


// ModifierLockType table
typedef TypeTable<ModifierLockType> TypeTable_ModifierLockType;
static const TypeTable_ModifierLockType g_modifierLockTypeTable[] = {
    { ModifierLockType_Lock0, _T("lock0") },
    { ModifierLockType_Lock1, _T("lock1") },
    { ModifierLockType_Lock2, _T("lock2") },
    { ModifierLockType_Lock3, _T("lock3") },
    { ModifierLockType_Lock4, _T("lock4") },
    { ModifierLockType_Lock5, _T("lock5") },
    { ModifierLockType_Lock6, _T("lock6") },
    { ModifierLockType_Lock7, _T("lock7") },
    { ModifierLockType_Lock8, _T("lock8") },
    { ModifierLockType_Lock9, _T("lock9") },
};


// stream output
tostream &operator<<(tostream &i_ost, ModifierLockType i_data)
{
    tstring name;
    if (getTypeName(&name, i_data,
                    g_modifierLockTypeTable, NUMBER_OF(g_modifierLockTypeTable)))
        i_ost << name;
    else
        i_ost << _T("(ModifierLockType internal error)");
    return i_ost;
}


// get value of ModifierLockType
bool getTypeValue(ModifierLockType *o_type, const tstring &i_name)
{
    return getTypeValue(o_type, i_name, g_modifierLockTypeTable,
                        NUMBER_OF(g_modifierLockTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ShowCommandType


// ShowCommandType table
typedef TypeTable<ShowCommandType> TypeTable_ShowCommandType;
static const TypeTable_ShowCommandType g_showCommandTypeTable[] = {
    { ShowCommandType_hide,            _T("hide")            },
    { ShowCommandType_maximize,        _T("maximize")        },
    { ShowCommandType_minimize,        _T("minimize")        },
    { ShowCommandType_restore,         _T("restore")         },
    { ShowCommandType_show,            _T("show")            },
    { ShowCommandType_showDefault,     _T("showDefault")     },
    { ShowCommandType_showMaximized,   _T("showMaximized")   },
    { ShowCommandType_showMinimized,   _T("showMinimized")   },
    { ShowCommandType_showMinNoActive, _T("showMinNoActive") },
    { ShowCommandType_showNA,          _T("showNA")          },
    { ShowCommandType_showNoActivate,  _T("showNoActivate")  },
    { ShowCommandType_showNormal,      _T("showNormal")      },
};


// stream output
tostream &operator<<(tostream &i_ost, ShowCommandType i_data)
{
    tstring name;
    if (getTypeName(&name, i_data,
                    g_showCommandTypeTable, NUMBER_OF(g_showCommandTypeTable)))
        i_ost << name;
    else
        i_ost << _T("(ShowCommandType internal error)");
    return i_ost;
}


// get value of ShowCommandType
bool getTypeValue(ShowCommandType *o_type, const tstring &i_name)
{
    return getTypeValue(o_type, i_name, g_showCommandTypeTable,
                        NUMBER_OF(g_showCommandTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TargetWindowType


// ModifierLockType table
typedef TypeTable<TargetWindowType> TypeTable_TargetWindowType;
static const TypeTable_TargetWindowType g_targetWindowType[] = {
    { TargetWindowType_overlapped, _T("overlapped") },
    { TargetWindowType_mdi,        _T("mdi")        },
};


// stream output
tostream &operator<<(tostream &i_ost, TargetWindowType i_data)
{
    tstring name;
    if (getTypeName(&name, i_data,
                    g_targetWindowType, NUMBER_OF(g_targetWindowType)))
        i_ost << name;
    else
        i_ost << _T("(TargetWindowType internal error)");
    return i_ost;
}


// get value of TargetWindowType
bool getTypeValue(TargetWindowType *o_type, const tstring &i_name)
{
    return getTypeValue(o_type, i_name, g_targetWindowType,
                        NUMBER_OF(g_targetWindowType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// BooleanType


// BooleanType table
typedef TypeTable<BooleanType> TypeTable_BooleanType;
static const TypeTable_BooleanType g_booleanType[] = {
    { BooleanType_false, _T("false") },
    { BooleanType_true,  _T("true")  },
};


// stream output
tostream &operator<<(tostream &i_ost, BooleanType i_data)
{
    tstring name;
    if (getTypeName(&name, i_data, g_booleanType, NUMBER_OF(g_booleanType)))
        i_ost << name;
    else
        i_ost << _T("(BooleanType internal error)");
    return i_ost;
}


// get value of BooleanType
bool getTypeValue(BooleanType *o_type, const tstring &i_name)
{
    return getTypeValue(o_type, i_name, g_booleanType,
                        NUMBER_OF(g_booleanType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LogicalOperatorType


// LogicalOperatorType table
typedef TypeTable<LogicalOperatorType> TypeTable_LogicalOperatorType;
static const TypeTable_LogicalOperatorType g_logicalOperatorType[] = {
    { LogicalOperatorType_or, _T("||") },
    { LogicalOperatorType_and,  _T("&&")  },
};


// stream output
tostream &operator<<(tostream &i_ost, LogicalOperatorType i_data)
{
    tstring name;
    if (getTypeName(&name, i_data, g_logicalOperatorType,
                    NUMBER_OF(g_logicalOperatorType)))
        i_ost << name;
    else
        i_ost << _T("(LogicalOperatorType internal error)");
    return i_ost;
}


// get value of LogicalOperatorType
bool getTypeValue(LogicalOperatorType *o_type, const tstring &i_name)
{
    return getTypeValue(o_type, i_name, g_logicalOperatorType,
                        NUMBER_OF(g_logicalOperatorType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// WindowMonitorFromType


// WindowMonitorFromType table
typedef TypeTable<WindowMonitorFromType> TypeTable_WindowMonitorFromType;
static const TypeTable_WindowMonitorFromType g_windowMonitorFromType[] = {
    { WindowMonitorFromType_primary, _T("primary") },
    { WindowMonitorFromType_current, _T("current") },
};


// stream output
tostream &operator<<(tostream &i_ost, WindowMonitorFromType i_data)
{
    tstring name;
    if (getTypeName(&name, i_data, g_windowMonitorFromType,
                    NUMBER_OF(g_windowMonitorFromType)))
        i_ost << name;
    else
        i_ost << _T("(WindowMonitorFromType internal error)");
    return i_ost;
}


// get value of WindowMonitorFromType
bool getTypeValue(WindowMonitorFromType *o_type, const tstring &i_name)
{
    return getTypeValue(o_type, i_name, g_windowMonitorFromType,
                        NUMBER_OF(g_windowMonitorFromType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// std::list<tstringq>


/// stream output
tostream &operator<<(tostream &i_ost, const std::list<tstringq> &i_data)
{
    for (std::list<tstringq>::const_iterator
            i = i_data.begin(); i != i_data.end(); ++ i) {
        i_ost << *i << _T(", ");
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
bool getSuitableWindow(FunctionParam *i_param, HWND *o_hwnd)
{
    if (!i_param->m_isPressed)
        return false;
    *o_hwnd = getToplevelWindow(i_param->m_hwnd, nullptr);
    if (!*o_hwnd)
        return false;
    return true;
}

//
bool getSuitableMdiWindow(WindowSystem *ws, FunctionParam *i_param, HWND *o_hwnd,
                          TargetWindowType *io_twt,
                          RECT *o_rcWindow = nullptr, RECT *o_rcParent = nullptr)
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
            WindowRect wr;
            if (ws->getWindowRect((WindowSystem::WindowHandle)*o_hwnd, &wr)) {
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
            WindowRect wr;
            if (ws->getChildWindowRect((WindowSystem::WindowHandle)*o_hwnd, &wr)) {
                o_rcWindow->left = wr.left;
                o_rcWindow->top = wr.top;
                o_rcWindow->right = wr.right;
                o_rcWindow->bottom = wr.bottom;
            }
        }
        if (o_rcParent) {
            WindowRect wr;
            if (ws->getClientRect(ws->getParent((WindowSystem::WindowHandle)*o_hwnd), &wr)) {
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
        if (text == nullptr || m_buf != text)
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
    const _TCHAR *i_data, int *o_retval)
{
    size_t len = m_buf.size();
    len += _tcslen(i_data) + 3;

    HGLOBAL hdata = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                                len * sizeof(_TCHAR));
    if (!hdata)
        return nullptr;
    _TCHAR *dataNew = reinterpret_cast<_TCHAR *>(GlobalLock(hdata));
    *dataNew = _T('\0');
    if (!m_buf.empty())
        _tcscpy(dataNew, m_buf.c_str());

    len = _tcslen(i_data);
    if (3 <= len &&
            i_data[len - 2] == _T('\r') && i_data[len - 1] == _T('\n')) {
        _tcscat(dataNew, i_data);
        len = _tcslen(dataNew);
        dataNew[len - 2] = _T('\0'); // chomp
        *o_retval = 2;
    } else if (len == 0) {
        _tcscat(dataNew, _T("\r\n"));
        *o_retval = 1;
    } else {
        _tcscat(dataNew, i_data);
        *o_retval = 0;
    }

    m_buf = dataNew;

    GlobalUnlock(hdata);
    return hdata;
}


// EmacsEditKillLinePred
int Engine::EmacsEditKillLine::pred()
{
    HGLOBAL g;
    const _TCHAR *text = clipboardGetText(&g);
    int retval;
    HGLOBAL hdata = makeNewKillLineBuf(text ? text : _T(""), &retval);
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
    Acquire a(&m_cs);

    Command_ShellExecute *fd =
        reinterpret_cast<Command_ShellExecute *>(
            m_afShellExecute->m_functionData);

    int r = m_windowSystem->shellExecute(
                fd->m_operation.eval().empty() ? _T("open") : fd->m_operation.eval(),
                fd->m_file.eval(),
                fd->m_parameters.eval(),
                fd->m_directory.eval(),
                static_cast<int>(fd->m_showCommand));
    if (32 < r)
        return; // success

    typedef TypeTable<int> ErrorTable;
    static const ErrorTable errorTable[] = {
        { 0, _T("The operating system is out of memory or resources.") },
        { ERROR_FILE_NOT_FOUND, _T("The specified file was not found.") },
        { ERROR_PATH_NOT_FOUND, _T("The specified path was not found.") },
        { ERROR_BAD_FORMAT, _T("The .exe file is invalid ")
          _T("(non-Win32R .exe or error in .exe image).") },
        { SE_ERR_ACCESSDENIED,
          _T("The operating system denied access to the specified file.") },
        { SE_ERR_ASSOCINCOMPLETE,
          _T("The file name association is incomplete or invalid.") },
        { SE_ERR_DDEBUSY,
          _T("The DDE transaction could not be completed ")
          _T("because other DDE transactions were being processed. ") },
        { SE_ERR_DDEFAIL, _T("The DDE transaction failed.") },
        { SE_ERR_DDETIMEOUT, _T("The DDE transaction could not be completed ")
          _T("because the request timed out.") },
        { SE_ERR_DLLNOTFOUND,
          _T("The specified dynamic-link library was not found.") },
        { SE_ERR_FNF, _T("The specified file was not found.") },
        { SE_ERR_NOASSOC, _T("There is no application associated ")
          _T("with the given file name extension.") },
        { SE_ERR_OOM,
          _T("There was not enough memory to complete the operation.") },
        { SE_ERR_PNF, _T("The specified path was not found.") },
        { SE_ERR_SHARE, _T("A sharing violation occurred.") },
    };

    tstring errorMessage(_T("Unknown error."));
    getTypeName(&errorMessage, r, errorTable, NUMBER_OF(errorTable));

    Acquire b(&m_log, 0);
    m_log << _T("error: ") << fd << _T(": ") << errorMessage << std::endl;
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
void Engine::funcWindowRaise(FunctionParam *i_param,
                             TargetWindowType i_twt)
{
    HWND hwnd;
    if (!getSuitableMdiWindow(m_windowSystem, i_param, &hwnd, &i_twt))
        return;
    m_windowSystem->setWindowZOrder((WindowSystem::WindowHandle)hwnd, ZOrder::Top);
}

// lower window
void Engine::funcWindowLower(FunctionParam *i_param, TargetWindowType i_twt)
{
    HWND hwnd;
    if (!getSuitableMdiWindow(m_windowSystem, i_param, &hwnd, &i_twt))
        return;
    m_windowSystem->setWindowZOrder((WindowSystem::WindowHandle)hwnd, ZOrder::Bottom);
}

// minimize window
void Engine::funcWindowMinimize(FunctionParam *i_param, TargetWindowType i_twt)
{
    HWND hwnd;
    if (!getSuitableMdiWindow(m_windowSystem, i_param, &hwnd, &i_twt))
        return;
    
    bool isIconic = (m_windowSystem->getShowCommand((WindowSystem::WindowHandle)hwnd) == WindowShowCmd::Minimized);
    m_windowSystem->postMessage((WindowSystem::WindowHandle)hwnd, WM_SYSCOMMAND,
                isIconic ? SC_RESTORE : SC_MINIMIZE, 0);
}

// maximize window
void Engine::funcWindowMaximize(FunctionParam *i_param, TargetWindowType i_twt)
{
    HWND hwnd;
    if (!getSuitableMdiWindow(m_windowSystem, i_param, &hwnd, &i_twt))
        return;
    
    bool isZoomed = (m_windowSystem->getShowCommand((WindowSystem::WindowHandle)hwnd) == WindowShowCmd::Maximized);
    m_windowSystem->postMessage((WindowSystem::WindowHandle)hwnd, WM_SYSCOMMAND,
                isZoomed ? SC_RESTORE : SC_MAXIMIZE, 0);
}

// maximize horizontally or virtically
void Engine::funcWindowHVMaximize(FunctionParam *i_param,
                                  BooleanType i_isHorizontal,
                                  TargetWindowType i_twt)
{
    HWND hwnd;
    RECT rc, rcd;
    if (!getSuitableMdiWindow(m_windowSystem, i_param, &hwnd, &i_twt, &rc, &rcd))
        return;

    int x = rc.left;
    int y = rc.top;
    int w = rcWidth(&rc);
    int h = rcHeight(&rc);

    if (i_isHorizontal) {
        x = rcd.left;
        w = rcWidth(&rcd);
    } else {
        y = rcd.top;
        h = rcHeight(&rcd);
    }
    asyncMoveWindow(hwnd, x, y, w, h);
}

// close window
void Engine::funcWindowClose(FunctionParam *i_param, TargetWindowType i_twt)
{
    HWND hwnd;
    if (!getSuitableMdiWindow(m_windowSystem, i_param, &hwnd, &i_twt))
        return;
    m_windowSystem->postMessage((WindowSystem::WindowHandle)hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
}

// toggle top-most flag of the window
void Engine::funcWindowToggleTopMost(FunctionParam *i_param)
{
    HWND hwnd;
    if (!getSuitableWindow(i_param, &hwnd))
        return;

    ZOrder order = m_windowSystem->isWindowTopMost((WindowSystem::WindowHandle)hwnd) 
        ? ZOrder::NoTopMost 
        : ZOrder::TopMost;
    
    m_windowSystem->setWindowZOrder((WindowSystem::WindowHandle)hwnd, order);
}

// identify the window
void Engine::funcWindowIdentify(FunctionParam *i_param)
{
    if (!i_param->m_isPressed)
        return;

    tstring className = m_windowSystem->getClassName((WindowSystem::WindowHandle)i_param->m_hwnd);
    bool ok = false;
    if (!className.empty()) {
        if (_tcsicmp(className.c_str(), _T("ConsoleWindowClass")) == 0) {
            tstring titleName = m_windowSystem->getTitleName((WindowSystem::WindowHandle)i_param->m_hwnd);
            {
                Acquire a(&m_log, 1);
                m_log << _T("HWND:\t") << std::hex
                << reinterpret_cast<ULONG_PTR>(i_param->m_hwnd)
                << std::dec << std::endl;
            }
            Acquire a(&m_log, 0);
            m_log << _T("CLASS:\t") << className << std::endl;
            m_log << _T("TITLE:\t") << titleName << std::endl;

            HWND hwnd = getToplevelWindow(i_param->m_hwnd, nullptr);
            WindowRect rc;
            m_windowSystem->getWindowRect((WindowSystem::WindowHandle)hwnd, &rc);
            m_log << _T("Toplevel Window Position/Size: (")
            << rc.left << _T(", ") << rc.top << _T(") / (")
            << (rc.right - rc.left) << _T("x") << (rc.bottom - rc.top)
            << _T(")") << std::endl;

            m_windowSystem->getWorkArea(&rc);
            m_log << _T("Desktop Window Position/Size: (")
            << rc.left << _T(", ") << rc.top << _T(") / (")
            << (rc.right - rc.left) << _T("x") << (rc.bottom - rc.top)
            << _T(")") << std::endl;

            m_log << std::endl;
            ok = true;
        }
    }
    if (!ok) {
        UINT WM_MAYU_MESSAGE = m_windowSystem->registerWindowMessage(
                                   addSessionId(WM_MAYU_MESSAGE_NAME).c_str());
        CHECK_TRUE( m_windowSystem->postMessage((WindowSystem::WindowHandle)i_param->m_hwnd, WM_MAYU_MESSAGE,
                                MayuMessage_notifyName, 0) );
    }
}

// set alpha blending parameter to the window
void Engine::funcWindowSetAlpha(FunctionParam *i_param, int i_alpha)
{
    HWND hwnd;
    if (!getSuitableWindow(i_param, &hwnd))
        return;

    if (i_alpha < 0) {    // remove all alpha
        for (WindowsWithAlpha::iterator i = m_windowsWithAlpha.begin();
                i != m_windowsWithAlpha.end(); ++ i) {
            m_windowSystem->setWindowLayered((WindowSystem::WindowHandle)*i, false);
            m_windowSystem->redrawWindow((WindowSystem::WindowHandle)*i);
        }
        m_windowsWithAlpha.clear();
    } else {
        if (m_windowSystem->isWindowLayered((WindowSystem::WindowHandle)hwnd)) {    // remove alpha
            WindowsWithAlpha::iterator
            i = std::find(m_windowsWithAlpha.begin(), m_windowsWithAlpha.end(),
                          hwnd);
            if (i == m_windowsWithAlpha.end())
                return;    // already layered by the application

            m_windowsWithAlpha.erase(i);

            m_windowSystem->setWindowLayered((WindowSystem::WindowHandle)hwnd, false);
        } else {    // add alpha
            m_windowSystem->setWindowLayered((WindowSystem::WindowHandle)hwnd, true);
            i_alpha %= 101;
            if (!m_windowSystem->setLayeredWindowAttributes((WindowSystem::WindowHandle)hwnd, 0,
                                            (unsigned char)(255 * i_alpha / 100), LWA_ALPHA)) {
                Acquire a(&m_log, 0);
                m_log << _T("error: &WindowSetAlpha(") << i_alpha
                << _T(") failed for HWND: ") << std::hex
                << reinterpret_cast<ULONG_PTR>(hwnd) << std::dec << std::endl;
                return;
            }
            m_windowsWithAlpha.push_front(hwnd);
        }
        m_windowSystem->redrawWindow((WindowSystem::WindowHandle)hwnd);
    }
}


// redraw the window
void Engine::funcWindowRedraw(FunctionParam *i_param)
{
    HWND hwnd;
    if (!getSuitableWindow(i_param, &hwnd))
        return;
    m_windowSystem->redrawWindow((WindowSystem::WindowHandle)hwnd);
}

// move window to ...
void Engine::funcWindowMoveTo(FunctionParam *i_param, GravityType i_gravityType,
                              int i_dx, int i_dy, TargetWindowType i_twt)
{
    HWND hwnd;
    RECT rc, rcd;
    if (!getSuitableMdiWindow(m_windowSystem, i_param, &hwnd, &i_twt, &rc, &rcd))
        return;

    int x = rc.left + i_dx;
    int y = rc.top + i_dy;

    if (i_gravityType & GravityType_N)
        y = i_dy + rcd.top;
    if (i_gravityType & GravityType_E)
        x = i_dx + rcd.right - rcWidth(&rc);
    if (i_gravityType & GravityType_W)
        x = i_dx + rcd.left;
    if (i_gravityType & GravityType_S)
        y = i_dy + rcd.bottom - rcHeight(&rc);
    asyncMoveWindow(hwnd, x, y);
}

// move window
void Engine::funcWindowMove(FunctionParam *i_param, int i_dx, int i_dy,
                            TargetWindowType i_twt)
{
    HWND hwnd;
    RECT rc, rcd;
    if (!getSuitableMdiWindow(m_windowSystem, i_param, &hwnd, &i_twt, &rc, &rcd))
        return;
    asyncMoveWindow(hwnd, rc.left + i_dx, rc.top + i_dy);
}

// maximize window horizontally
void Engine::funcWindowHMaximize(FunctionParam *i_param, TargetWindowType i_twt)
{
    funcWindowHVMaximize(i_param, BooleanType_true, i_twt);
}

// maximize window vertically
void Engine::funcWindowVMaximize(FunctionParam *i_param, TargetWindowType i_twt)
{
    funcWindowHVMaximize(i_param, BooleanType_false, i_twt);
}


// move window visibly
void Engine::funcWindowMoveVisibly(FunctionParam *i_param,
                                   TargetWindowType i_twt)
{
    HWND hwnd;
    RECT rc, rcd;
    if (!getSuitableMdiWindow(m_windowSystem, i_param, &hwnd, &i_twt, &rc, &rcd))
        return;

    int x = rc.left;
    int y = rc.top;
    if (rc.left < rcd.left)
        x = rcd.left;
    else if (rcd.right < rc.right)
        x = rcd.right - rcWidth(&rc);
    if (rc.top < rcd.top)
        y = rcd.top;
    else if (rcd.bottom < rc.bottom)
        y = rcd.bottom - rcHeight(&rc);
    asyncMoveWindow(hwnd, x, y);
}


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
void Engine::funcWindowMonitorTo(
    FunctionParam *i_param, WindowMonitorFromType i_fromType, int i_monitor,
    BooleanType i_adjustPos, BooleanType i_adjustSize)
{
    HWND hwnd;
    if (! getSuitableWindow(i_param, &hwnd))
        return;

    HMONITOR hmonCur;
    hmonCur = monitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    EnumDisplayMonitorsForWindowMonitorToParam ep(hmonCur);
    enumDisplayMonitors(nullptr, nullptr, enumDisplayMonitorsForWindowMonitorTo,
                        reinterpret_cast<LPARAM>(&ep));
    if (ep.m_monitors.size() < 1 ||
            ep.m_primaryMonitorIdx < 0 || ep.m_currentMonitorIdx < 0)
        return;

    int targetIdx = 0;
    switch (i_fromType) {
    case WindowMonitorFromType_primary:
        targetIdx = (ep.m_primaryMonitorIdx + i_monitor) % (int)ep.m_monitors.size();
        break;

    case WindowMonitorFromType_current:
        targetIdx = (ep.m_currentMonitorIdx + i_monitor) % (int)ep.m_monitors.size();
        break;
    }
    if (ep.m_currentMonitorIdx == targetIdx)
        return;

    RECT rcCur, rcTarget, rcWin;
    rcCur = ep.m_monitorinfos[ep.m_currentMonitorIdx].rcWork;
    rcTarget = ep.m_monitorinfos[targetIdx].rcWork;
    GetWindowRect(hwnd, &rcWin);

    int x = rcTarget.left + (rcWin.left - rcCur.left);
    int y = rcTarget.top + (rcWin.top - rcCur.top);
    int w = rcWidth(&rcWin);
    int h = rcHeight(&rcWin);

    if (i_adjustPos) {
        if (x + w > rcTarget.right)
            x = rcTarget.right - w;
        if (x < rcTarget.left)
            x = rcTarget.left;
        if (w > rcWidth(&rcTarget)) {
            x = rcTarget.left;
            w = rcWidth(&rcTarget);
        }

        if (y + h > rcTarget.bottom)
            y = rcTarget.bottom - h;
        if (y < rcTarget.top)
            y = rcTarget.top;
        if (h > rcHeight(&rcTarget)) {
            y = rcTarget.top;
            h = rcHeight(&rcTarget);
        }
    }

    if (i_adjustPos && i_adjustSize) {
        if (m_windowSystem->getShowCommand((WindowSystem::WindowHandle)hwnd) == WindowShowCmd::Maximized)
            m_windowSystem->postMessage((WindowSystem::WindowHandle)hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        asyncMoveWindow(hwnd, x, y, w, h);
    } else {
        asyncMoveWindow(hwnd, x, y);
    }
}

/// move window to other monitor
void Engine::funcWindowMonitor(
    FunctionParam *i_param, int i_monitor,
    BooleanType i_adjustPos, BooleanType i_adjustSize)
{
    funcWindowMonitorTo(i_param, WindowMonitorFromType_primary, i_monitor,
                        i_adjustPos, i_adjustSize);
}


//
void Engine::funcWindowClingToLeft(FunctionParam *i_param,
                                   TargetWindowType i_twt)
{
    funcWindowMoveTo(i_param, GravityType_W, 0, 0, i_twt);
}

//
void Engine::funcWindowClingToRight(FunctionParam *i_param,
                                    TargetWindowType i_twt)
{
    funcWindowMoveTo(i_param, GravityType_E, 0, 0, i_twt);
}

//
void Engine::funcWindowClingToTop(FunctionParam *i_param,
                                  TargetWindowType i_twt)
{
    funcWindowMoveTo(i_param, GravityType_N, 0, 0, i_twt);
}

//
void Engine::funcWindowClingToBottom(FunctionParam *i_param,
                                     TargetWindowType i_twt)
{
    funcWindowMoveTo(i_param, GravityType_S, 0, 0, i_twt);
}

// resize window to
void Engine::funcWindowResizeTo(FunctionParam *i_param, int i_width,
                                int i_height, TargetWindowType i_twt)
{
    HWND hwnd;
    RECT rc, rcd;
    if (!getSuitableMdiWindow(m_windowSystem, i_param, &hwnd, &i_twt, &rc, &rcd))
        return;

    if (i_width == 0)
        i_width = rcWidth(&rc);
    else if (i_width < 0)
        i_width += rcWidth(&rcd);

    if (i_height == 0)
        i_height = rcHeight(&rc);
    else if (i_height < 0)
        i_height += rcHeight(&rcd);

    asyncResize(hwnd, i_width, i_height);
}

// move the mouse cursor
void Engine::funcMouseMove(FunctionParam *i_param, int i_dx, int i_dy)
{
    if (!i_param->m_isPressed)
        return;
    WindowPoint pt;
    m_windowSystem->getCursorPos(&pt);
    m_windowSystem->setCursorPos(pt.x + i_dx, pt.y + i_dy);
}

// send a mouse-wheel-message to Windows
void Engine::funcMouseWheel(FunctionParam *i_param, int i_delta)
{
    if (!i_param->m_isPressed)
        return;

    if (m_inputInjector) {
        KEYBOARD_INPUT_DATA kid;
        kid.UnitId = 0;
        kid.MakeCode = 10; // Generic Wheel
        kid.Flags = KEYBOARD_INPUT_DATA::E1; // Mouse event
        kid.Reserved = 0;
        kid.ExtraInformation = static_cast<unsigned long>(i_delta);

        InjectionContext ctx;
        ctx.isDragging = false;

        m_inputInjector->inject(&kid, ctx);
    }
}
void Engine::funcClipboardChangeCase(FunctionParam *i_param,
                                     BooleanType i_doesConvertToUpperCase)
{
    if (!i_param->m_isPressed)
        return;

    tstring text = m_windowSystem->getClipboardText();
    if (text.empty())
        return;

    for (size_t i = 0; i < text.size(); ++i) {
        _TCHAR c = text[i];
        // Assuming _istlead check is handled if needed, but tstring might be wstring/string
        // If tstring is std::string (MBCS), we need _istlead.
        // If tstring is std::wstring (Unicode), we don't.
        // WindowSystem::getClipboardText handles TCHAR.
#ifndef _UNICODE
        if (_istlead(c)) {
            ++i; // Skip next char as it is trail byte
            continue;
        }
#endif
        text[i] = i_doesConvertToUpperCase ? _totupper(c) : _totlower(c);
    }
    
    m_windowSystem->setClipboardText(text);
}

// convert the contents of the Clipboard to upper case
void Engine::funcClipboardUpcaseWord(FunctionParam *i_param)
{
    funcClipboardChangeCase(i_param, BooleanType_true);
}

// convert the contents of the Clipboard to lower case
void Engine::funcClipboardDowncaseWord(FunctionParam *i_param)
{
    funcClipboardChangeCase(i_param, BooleanType_false);
}

// set the contents of the Clipboard to the string
void Engine::funcClipboardCopy(FunctionParam *i_param, const StrExprArg &i_text)
{
    if (!i_param->m_isPressed)
        return;
    
    m_windowSystem->setClipboardText(i_text.eval());
}

//
void Engine::funcEmacsEditKillLinePred(
    FunctionParam *i_param, const KeySeq *i_keySeq1, const KeySeq *i_keySeq2)
{
    m_emacsEditKillLine.m_doForceReset = false;
    if (!i_param->m_isPressed)
        return;

    int r = m_emacsEditKillLine.pred();
    const KeySeq *keySeq;
    if (r == 1)
        keySeq = i_keySeq1;
    else if (r == 2)
        keySeq = i_keySeq2;
    else // r == 0
        return;
    ASSERT(keySeq);
    generateKeySeqEvents(i_param->m_c, keySeq, Part_all);
}

//
void Engine::funcEmacsEditKillLineFunc(FunctionParam *i_param)
{
    if (!i_param->m_isPressed)
        return;
    m_emacsEditKillLine.func();
    m_emacsEditKillLine.m_doForceReset = false;
}

// clear log
void Engine::funcLogClear(FunctionParam *i_param)
{
    if (!i_param->m_isPressed)
        return;
    m_windowSystem->postMessage(getAssociatedWndow(), WM_APP_engineNotify,
                EngineNotify_clearLog, 0);
}

// recenter
void Engine::funcRecenter(FunctionParam *i_param)
{
    if (m_hwndFocus) {
        UINT WM_MAYU_MESSAGE = m_windowSystem->registerWindowMessage(
                                   addSessionId(WM_MAYU_MESSAGE_NAME).c_str());
        m_windowSystem->postMessage((WindowSystem::WindowHandle)m_hwndFocus, WM_MAYU_MESSAGE, MayuMessage_funcRecenter, 0);
    }
}

// set IME open status
void Engine::funcSetImeStatus(FunctionParam *i_param, ToggleType i_toggle)
{
    if (!i_param->m_isPressed)
        return;
    if (m_hwndFocus) {
        UINT WM_MAYU_MESSAGE = m_windowSystem->registerWindowMessage(
                                   addSessionId(WM_MAYU_MESSAGE_NAME).c_str());
        int status = -1;
        switch (i_toggle) {
        case ToggleType_toggle:
            status = -1;
            break;
        case ToggleType_off:
            status = 0;
            break;
        case ToggleType_on:
            status = 1;
            break;
        }
        m_windowSystem->postMessage((WindowSystem::WindowHandle)m_hwndFocus, WM_MAYU_MESSAGE, MayuMessage_funcSetImeStatus, status);
    }
}

// set IME open status
void Engine::funcSetImeString(FunctionParam *i_param, const StrExprArg &i_data)
{
    if (!i_param->m_isPressed)
        return;
    if (m_hwndFocus) {
        UINT WM_MAYU_MESSAGE = m_windowSystem->registerWindowMessage(
                                   addSessionId(WM_MAYU_MESSAGE_NAME).c_str());
        m_windowSystem->postMessage((WindowSystem::WindowHandle)m_hwndFocus, WM_MAYU_MESSAGE, MayuMessage_funcSetImeString, i_data.eval().size() * sizeof(_TCHAR));

        unsigned int len = 0;
        m_windowSystem->disconnectNamedPipe(m_hookPipe);
        m_windowSystem->connectNamedPipe(m_hookPipe, nullptr);
        m_windowSystem->writeFile(m_hookPipe, i_data.eval().c_str(),
                          (unsigned int)(i_data.eval().size() * sizeof(_TCHAR)),
                          &len, nullptr);

        //FlushFileBuffers(m_hookPipe);
    }
}

// Direct SSTP Server
class DirectSSTPServer
{
public:
    tstring m_path;
    HWND m_hwnd;
    tstring m_name;
    tstring m_keroname;

public:
    DirectSSTPServer()
            : m_hwnd(nullptr) {
    }
};


class ParseDirectSSTPData
{
    typedef std::match_results<const char*> MR;

public:
    typedef std::map<tstring, DirectSSTPServer> DirectSSTPServers;

private:
    DirectSSTPServers *m_directSSTPServers;

public:
    // constructor
    ParseDirectSSTPData(DirectSSTPServers *i_directSSTPServers)
            : m_directSSTPServers(i_directSSTPServers) {
    }

    bool operator()(const MR& i_what) {
#ifdef _UNICODE
        tstring id(to_wstring(std::string(i_what[1].first, i_what[1].second)));
        tstring member(to_wstring(std::string(i_what[2].first, i_what[2].second)));
        tstring value(to_wstring(std::string(i_what[3].first, i_what[3].second)));
#else
        tstring id(i_what[1].first, i_what[1].second);
        tstring member(i_what[2].first, i_what[2].second);
        tstring value(i_what[3].first, i_what[3].second);
#endif

        if (member == _T("path"))
            (*m_directSSTPServers)[id].m_path = value;
        else if (member == _T("hwnd"))
            (*m_directSSTPServers)[id].m_hwnd =
                reinterpret_cast<HWND>((LONG_PTR)_ttoi64(value.c_str()));
        else if (member == _T("name"))
            (*m_directSSTPServers)[id].m_name = value;
        else if (member == _T("keroname"))
            (*m_directSSTPServers)[id].m_keroname = value;
        return true;
    }
};

// Direct SSTP
void Engine::funcDirectSSTP(FunctionParam *i_param,
                            const tregex &i_name,
                            const StrExprArg &i_protocol,
                            const std::list<tstringq> &i_headers)
{
    if (!i_param->m_isPressed)
        return;

    // check Direct SSTP server exist ?
    if (void* hm = m_windowSystem->openMutex(_T("sakura")))
        m_windowSystem->closeHandle(hm);
    else {
        Acquire a(&m_log, 0);
        m_log << _T(" Error(1): Direct SSTP server does not exist.");
        return;
    }

    void* hfm = m_windowSystem->openFileMapping(_T("Sakura"));
    if (!hfm) {
        Acquire a(&m_log, 0);
        m_log << _T(" Error(2): Direct SSTP server does not provide data.");
        return;
    }

    char *data =
        reinterpret_cast<char *>(m_windowSystem->mapViewOfFile(hfm));
    if (!data) {
        m_windowSystem->closeHandle(hfm);
        Acquire a(&m_log, 0);
        m_log << _T(" Error(3): Direct SSTP server does not provide data.");
        return;
    }

    long length = *(long *)data;
    const char *begin = data + 4;
    const char *end = data + length;
    std::regex getSakura("([0-9a-fA-F]{32})\\.([^\x01]+)\x01(.*?)\r\n");

    ParseDirectSSTPData::DirectSSTPServers servers;
    std::regex_iterator<const char*>
    it(begin, end, getSakura), last;
    for (; it != last; ++it)
        ((ParseDirectSSTPData)(&servers))(*it);

    // make request
    tstring request;
    if (!i_protocol.eval().size())
        request += _T("NOTIFY SSTP/1.1");
    else
        request += i_protocol.eval();
    request += _T("\r\n");

    bool hasSender = false;
    for (std::list<tstringq>::const_iterator
            i = i_headers.begin(); i != i_headers.end(); ++ i) {
        if (_tcsnicmp(_T("Charset"), i->c_str(), 7) == 0 ||
                _tcsnicmp(_T("Hwnd"),    i->c_str(), 4) == 0)
            continue;
        if (_tcsnicmp(_T("Sender"), i->c_str(), 6) == 0)
            hasSender = true;
        request += i->c_str();
        request += _T("\r\n");
    }

    if (!hasSender) {
        request += _T("Sender: ");
        request += loadString(IDS_mayu);
        request += _T("\r\n");
    }

    _TCHAR buf[100];
    _sntprintf(buf, NUMBER_OF(buf), _T("HWnd: %Iu\r\n"),
               reinterpret_cast<ULONG_PTR>(m_hwndAssocWindow));
    request += buf;

#ifdef _UNICODE
    request += _T("Charset: UTF-8\r\n");
#else
    request += _T("Charset: Shift_JIS\r\n");
#endif
    request += _T("\r\n");

#ifdef _UNICODE
    std::string request_UTF_8 = to_UTF_8(request);
#endif

    // send request to Direct SSTP Server which matches i_name;
    for (ParseDirectSSTPData::DirectSSTPServers::iterator
            i = servers.begin(); i != servers.end(); ++ i) {
        tsmatch what;
        if (std::regex_match(i->second.m_name, what, i_name)) {
            COPYDATASTRUCT cd;
            cd.dwData = 9801;
#ifdef _UNICODE
            cd.cbData = (DWORD)request_UTF_8.size();
            cd.lpData = (void *)request_UTF_8.c_str();
#else
            cd.cbData = (DWORD)request.size();
            cd.lpData = (void *)request.c_str();
#endif
            uintptr_t result;
            m_windowSystem->sendMessageTimeout((WindowSystem::WindowHandle)i->second.m_hwnd, WM_COPYDATA,
                               reinterpret_cast<uintptr_t>(m_hwndAssocWindow),
                               reinterpret_cast<intptr_t>(&cd),
                               SMTO_ABORTIFHUNG | SMTO_BLOCK, 5000, &result);
        }
    }

    m_windowSystem->unmapViewOfFile(data);
    m_windowSystem->closeHandle(hfm);
}


namespace shu
{
class PlugIn
{
    enum Type {
        Type_A,
        Type_W
    };

private:
    WindowSystem* m_ws;
    void* m_dll;
    void* m_func;
    Type m_type;
    tstringq m_funcParam;

public:
    PlugIn(WindowSystem* ws) : m_ws(ws), m_dll(nullptr), m_func(nullptr) {
    }

    ~PlugIn() {
        if (m_dll)
            m_ws->freeLibrary(m_dll);
    }

    bool load(const tstringq &i_dllName, const tstringq &i_funcName,
              const tstringq &i_funcParam, tomsgstream &i_log) {
        m_dll = m_ws->loadLibrary((_T("Plugins\\") + i_dllName).c_str());
        if (!m_dll) {
            m_dll = m_ws->loadLibrary((_T("Plugin\\") + i_dllName).c_str());
            if (!m_dll) {
                m_dll = m_ws->loadLibrary(i_dllName.c_str());
                if (!m_dll) {
                    Acquire a(&i_log);
                    i_log << std::endl;
                    i_log << _T("error: &PlugIn() failed to load ") << i_dllName << std::endl;
                    return false;
                }
            }
        }

        // get function
#ifdef UNICODE
#  define to_wstring
#else
#  define to_string
#endif
        m_type = Type_W;
        m_func = m_ws->getProcAddress(m_dll, to_string(_T("mayu") + i_funcName + _T("W")));
        if (!m_func) {
            m_type = Type_A;
            m_func
            = m_ws->getProcAddress(m_dll, to_string(_T("mayu") + i_funcName + _T("A")));
            if (!m_func) {
                m_func = m_ws->getProcAddress(m_dll, to_string(_T("mayu") + i_funcName));
                if (!m_func) {
                    m_func = m_ws->getProcAddress(m_dll, to_string(i_funcName));
                    if (!m_func) {
                        Acquire a(&i_log);
                        i_log << std::endl;
                        i_log << _T("error: &PlugIn() failed to find function: ")
                        << i_funcName << std::endl;
                        return false;
                    }
                }
            }
        }

        m_funcParam = i_funcParam;
        return true;
    }

    void exec() {
        if (!m_dll || !m_func) return;

        typedef void (WINAPI * PLUGIN_FUNCTION_A)(const char *i_arg);
        typedef void (WINAPI * PLUGIN_FUNCTION_W)(const wchar_t *i_arg);
        switch (m_type) {
        case Type_A:
            reinterpret_cast<PLUGIN_FUNCTION_A>(m_func)(to_string(m_funcParam).c_str());
            break;
        case Type_W:
            reinterpret_cast<PLUGIN_FUNCTION_W>(m_func)(to_wstring(m_funcParam).c_str());
            break;
        }
    }
#undef to_string
#undef to_wstring
};

static void plugInThread(void *i_plugin)
{
    PlugIn *plugin = static_cast<PlugIn *>(i_plugin);
    plugin->exec();
    delete plugin;
}
}

void Engine::funcPlugIn(FunctionParam *i_param,
                        const StrExprArg &i_dllName,
                        const StrExprArg &i_funcName,
                        const StrExprArg &i_funcParam,
                        BooleanType i_doesCreateThread)
{
    if (!i_param->m_isPressed)
        return;

    shu::PlugIn *plugin = new shu::PlugIn(m_windowSystem);
    if (!plugin->load(i_dllName.eval(), i_funcName.eval(), i_funcParam.eval(), m_log)) {
        delete plugin;
        return;
    }
    if (i_doesCreateThread) {
        if (_beginthread(shu::plugInThread, 0, plugin) == static_cast<uintptr_t>(-1)) {
            delete plugin;
            Acquire a(&m_log);
            m_log << std::endl;
            m_log << _T("error: &PlugIn() failed to create thread.");
        }
        return;
    } else
        plugin->exec();
}


void Engine::funcMouseHook(FunctionParam *i_param,
                           MouseHookType i_hookType, int i_hookParam)
{
    WindowPoint wp;
    m_windowSystem->getCursorPos(&wp);
    g_hookData->m_mousePos.x = wp.x;
    g_hookData->m_mousePos.y = wp.y;

    g_hookData->m_mouseHookType = i_hookType;
    g_hookData->m_mouseHookParam = i_hookParam;

    switch (i_hookType) {
    case MouseHookType_WindowMove: {
        // For this type, g_hookData->m_mouseHookParam means
        // target window type to move.
        HWND target;
        bool isMDI;

        // i_hooParam < 0 means target window to move is MDI.
        if (i_hookParam < 0)
            isMDI = true;
        else
            isMDI = false;

        // abs(i_hookParam) == 2: target is window under mouse cursor
        // otherwise: target is current focus window
        if (i_hookParam == 2 || i_hookParam == -2)
            target = (HWND)m_windowSystem->windowFromPoint(wp);
        else
            target = i_param->m_hwnd;

        g_hookData->m_hwndMouseHookTarget =
            (DWORD)((ULONG_PTR)getToplevelWindow(target, &isMDI));
        break;
    }
    default:
        g_hookData->m_hwndMouseHookTarget = 0;
        break;
    }
}


// cancel prefix
void Engine::funcCancelPrefix(FunctionParam *i_param)
{
    m_isPrefix = false;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

