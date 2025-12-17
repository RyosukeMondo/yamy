//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// function.cpp


#include "engine.h"
#include "hook.h"
#include "mayu.h"
#include "mayurc.h"
#include "misc.h"
#include "vkeytable.h"
// #include "windowstool.h" // Removed Win32 dependency
#include <algorithm>

#include "function_data.h"
#include "../../platform/windows/utf_conversion.h" // For conversion if needed (but prefer std::string)
#include "stringtool.h" // For strcasecmp_platform
#include <cstring>
#include <ostream>

using namespace yamy::platform;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TypeTable (Same as before)
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

static bool iequals(const char* a, const std::string& b) {
    return strcasecmp_platform(a, b.c_str()) == 0;
}

template <class T> static inline
bool getTypeValue(T *o_type, const std::string &i_name,
                  const TypeTable<T> *i_table, size_t i_n)
{
    for (size_t i = 0; i < i_n; ++ i)
        if (iequals(i_table[i].m_name, i_name)) {
            *o_type = i_table[i].m_type;
            return true;
        }
    return false;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// VKey (Same as before)
std::ostream &operator<<(std::ostream &i_ost, VKey i_data)
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
// ToWindowType (Same as before)
typedef TypeTable<ToWindowType> TypeTable_ToWindowType;
static const TypeTable_ToWindowType g_toWindowTypeTable[] = {
    { ToWindowType_toOverlappedWindow, "toOverlappedWindow" },
    { ToWindowType_toMainWindow,       "toMainWindow"       },
    { ToWindowType_toItself,           "toItself"           },
    { ToWindowType_toParentWindow,     "toParentWindow"     },
};

std::ostream &operator<<(std::ostream &i_ost, ToWindowType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_toWindowTypeTable, NUMBER_OF(g_toWindowTypeTable)))
        i_ost << name;
    else
        i_ost << static_cast<int>(i_data);
    return i_ost;
}

bool getTypeValue(ToWindowType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name,
                        g_toWindowTypeTable, NUMBER_OF(g_toWindowTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GravityType (Same as before)
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

std::ostream &operator<<(std::ostream &i_ost, GravityType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_gravityTypeTable, NUMBER_OF(g_gravityTypeTable)))
        i_ost << name;
    else
        i_ost << "(GravityType internal error)";
    return i_ost;
}

bool getTypeValue(GravityType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name,
                        g_gravityTypeTable, NUMBER_OF(g_gravityTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MouseHookType (Same as before)
typedef TypeTable<MouseHookType> TypeTable_MouseHookType;
static const TypeTable_MouseHookType g_mouseHookTypeTable[] = {
    { MouseHookType_None,  "None"  },
    { MouseHookType_Wheel,  "Wheel"  },
    { MouseHookType_WindowMove,  "WindowMove"  },
};

std::ostream &operator<<(std::ostream &i_ost, MouseHookType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_mouseHookTypeTable, NUMBER_OF(g_mouseHookTypeTable)))
        i_ost << name;
    else
        i_ost << "(MouseHookType internal error)";
    return i_ost;
}

bool getTypeValue(MouseHookType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_mouseHookTypeTable,
                        NUMBER_OF(g_mouseHookTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MayuDialogType (Same as before)
typedef TypeTable<MayuDialogType> TypeTable_MayuDialogType;
static const TypeTable_MayuDialogType g_mayuDialogTypeTable[] = {
    { MayuDialogType_investigate, "investigate"  },
    { MayuDialogType_log,         "log"          },
};

std::ostream &operator<<(std::ostream &i_ost, MayuDialogType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_mayuDialogTypeTable, NUMBER_OF(g_mayuDialogTypeTable)))
        i_ost << name;
    else
        i_ost << "(MayuDialogType internal error)";
    return i_ost;
}

bool getTypeValue(MayuDialogType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_mayuDialogTypeTable,
                        NUMBER_OF(g_mayuDialogTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ToggleType (Same as before)
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

std::ostream &operator<<(std::ostream &i_ost, ToggleType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data, g_toggleType, NUMBER_OF(g_toggleType)))
        i_ost << name;
    else
        i_ost << "(ToggleType internal error)";
    return i_ost;
}

bool getTypeValue(ToggleType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_toggleType,
                        NUMBER_OF(g_toggleType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ModifierLockType (Same as before)
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

std::ostream &operator<<(std::ostream &i_ost, ModifierLockType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_modifierLockTypeTable, NUMBER_OF(g_modifierLockTypeTable)))
        i_ost << name;
    else
        i_ost << "(ModifierLockType internal error)";
    return i_ost;
}

bool getTypeValue(ModifierLockType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_modifierLockTypeTable,
                        NUMBER_OF(g_modifierLockTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ShowCommandType (Same as before)
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

std::ostream &operator<<(std::ostream &i_ost, ShowCommandType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_showCommandTypeTable, NUMBER_OF(g_showCommandTypeTable)))
        i_ost << name;
    else
        i_ost << "(ShowCommandType internal error)";
    return i_ost;
}

bool getTypeValue(ShowCommandType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_showCommandTypeTable,
                        NUMBER_OF(g_showCommandTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TargetWindowType (Same as before)
typedef TypeTable<TargetWindowType> TypeTable_TargetWindowType;
static const TypeTable_TargetWindowType g_targetWindowType[] = {
    { TargetWindowType_overlapped, "overlapped" },
    { TargetWindowType_mdi,        "mdi"        },
};

std::ostream &operator<<(std::ostream &i_ost, TargetWindowType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data,
                    g_targetWindowType, NUMBER_OF(g_targetWindowType)))
        i_ost << name;
    else
        i_ost << "(TargetWindowType internal error)";
    return i_ost;
}

bool getTypeValue(TargetWindowType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_targetWindowType,
                        NUMBER_OF(g_targetWindowType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// BooleanType (Same as before)
typedef TypeTable<BooleanType> TypeTable_BooleanType;
static const TypeTable_BooleanType g_booleanType[] = {
    { BooleanType_false, "false" },
    { BooleanType_true,  "true"  },
};

std::ostream &operator<<(std::ostream &i_ost, BooleanType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data, g_booleanType, NUMBER_OF(g_booleanType)))
        i_ost << name;
    else
        i_ost << "(BooleanType internal error)";
    return i_ost;
}

bool getTypeValue(BooleanType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_booleanType,
                        NUMBER_OF(g_booleanType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LogicalOperatorType (Same as before)
typedef TypeTable<LogicalOperatorType> TypeTable_LogicalOperatorType;
static const TypeTable_LogicalOperatorType g_logicalOperatorType[] = {
    { LogicalOperatorType_or, "||" },
    { LogicalOperatorType_and,  "&&"  },
};

std::ostream &operator<<(std::ostream &i_ost, LogicalOperatorType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data, g_logicalOperatorType,
                    NUMBER_OF(g_logicalOperatorType)))
        i_ost << name;
    else
        i_ost << "(LogicalOperatorType internal error)";
    return i_ost;
}

bool getTypeValue(LogicalOperatorType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_logicalOperatorType,
                        NUMBER_OF(g_logicalOperatorType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// WindowMonitorFromType (Same as before)
typedef TypeTable<WindowMonitorFromType> TypeTable_WindowMonitorFromType;
static const TypeTable_WindowMonitorFromType g_windowMonitorFromType[] = {
    { WindowMonitorFromType_primary, "primary" },
    { WindowMonitorFromType_current, "current" },
};

std::ostream &operator<<(std::ostream &i_ost, WindowMonitorFromType i_data)
{
    std::string name;
    if (getTypeName(&name, i_data, g_windowMonitorFromType,
                    NUMBER_OF(g_windowMonitorFromType)))
        i_ost << name;
    else
        i_ost << "(WindowMonitorFromType internal error)";
    return i_ost;
}

bool getTypeValue(WindowMonitorFromType *o_type, const std::string &i_name)
{
    return getTypeValue(o_type, i_name, g_windowMonitorFromType,
                        NUMBER_OF(g_windowMonitorFromType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// std::list<stringq> (Same as before)
std::ostream &operator<<(std::ostream &i_ost, const std::list<stringq> &i_data)
{
    for (std::list<stringq>::const_iterator
            i = i_data.begin(); i != i_data.end(); ++ i) {
        i_ost << *i << ", ";
    }
    return i_ost;
}

std::ostream &operator<<(std::ostream &i_ost, const std::list<std::string> &i_data)
{
    for (std::list<std::string>::const_iterator
            i = i_data.begin(); i != i_data.end(); ++ i) {
        i_ost << *i << ", ";
    }
    return i_ost;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// FunctionData (Same as before)
FunctionData::~FunctionData()
{
}

std::ostream &operator<<(std::ostream &i_ost, const FunctionData *i_data)
{
    return i_data->output(i_ost);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// misc. functions


//
bool Engine::getSuitableWindow(IWindowSystem *ws, FunctionParam *i_param, WindowHandle *o_hwnd)
{
    if (!i_param->m_isPressed)
        return false;

    // Emulate getToplevelWindow using IWindowSystem
    WindowHandle top = i_param->m_hwnd;
    while (true) {
        if (!ws->isChild(top)) break;

        WindowHandle parent = ws->getParent(top);
        if (!parent) break;
        top = parent;
    }
    *o_hwnd = top;

    if (!*o_hwnd)
        return false;
    return true;
}

//
bool Engine::getSuitableMdiWindow(IWindowSystem *ws, FunctionParam *i_param, WindowHandle *o_hwnd,
                          TargetWindowType *io_twt,
                          Rect *o_rcWindow /*= nullptr*/, Rect *o_rcParent /*= nullptr*/)
{
    if (!i_param->m_isPressed)
        return false;

    bool isMdi = *io_twt == TargetWindowType_mdi;

    // Emulate getToplevelWindow with MDI support
    WindowHandle hwnd = i_param->m_hwnd;
    while (hwnd) {
        if (!ws->isChild(hwnd)) break;

        if (isMdi) {
            if (ws->isMDIChild(hwnd)) {
                *o_hwnd = hwnd;
                // Found MDI child
                goto found;
            }
        }

        WindowHandle parent = ws->getParent(hwnd);
        if (!parent) break;
        hwnd = parent;
    }
    // If not found MDI child, return toplevel
    if (isMdi) isMdi = false; // reset flag if MDI child not found
    *o_hwnd = hwnd;

found:
    *io_twt = isMdi ? TargetWindowType_mdi : TargetWindowType_overlapped;
    if (!*o_hwnd)
        return false;

    switch (*io_twt) {
    case TargetWindowType_overlapped:
        if (o_rcWindow) {
            if (!ws->getWindowRect(*o_hwnd, o_rcWindow)) {
                // handle error
            }
        }
        if (o_rcParent) {
            // Monitor info
            ws->getWorkArea(o_rcParent);
        }
        break;
    case TargetWindowType_mdi:
        if (o_rcWindow) {
            if (!ws->getChildWindowRect(*o_hwnd, o_rcWindow)) {
                // handle error
            }
        }
        if (o_rcParent) {
            WindowHandle parent = ws->getParent(*o_hwnd);
            if (!ws->getClientRect(parent, o_rcParent)) {
                // handle error
            }
        }
        break;
    }
    return true;
}


// EmacsEditKillLineFunc.
void Engine::EmacsEditKillLine::func(IWindowSystem* ws)
{
    if (!m_buf.empty()) {
        std::string text = ws->getClipboardText();
        if (m_buf != text)
            reset();
    }

    // Clear clipboard
    ws->setClipboardText("");
}


// EmacsEditKillLinePred
int Engine::EmacsEditKillLine::pred(IWindowSystem* ws)
{
    std::string text = ws->getClipboardText();

    // Logic from makeNewKillLineBuf
    int retval = 0;

    // m_buf += text + ...
    std::string dataNew = m_buf.empty() ? "" : m_buf;

    size_t len = text.length();
    if (3 <= len && text[len-2] == '\r' && text[len-1] == '\n') {
        dataNew += text;
        // chomp last 2
        dataNew = dataNew.substr(0, dataNew.length() - 2);
        retval = 2;
    } else if (len == 0) {
        dataNew += "\r\n";
        retval = 1;
    } else {
        dataNew += text;
        retval = 0;
    }

    m_buf = dataNew;
    return retval;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// functions

#include "../commands/cmd_shell_execute.h"
// shell execute
void Engine::shellExecute()
{
    Command_ShellExecute::executeOnMainThread(this);
}

// ... other commented out functions ...
