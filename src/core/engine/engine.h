#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine.h


#ifndef _ENGINE_H
#  define _ENGINE_H

#  include "multithread.h"
#  include "setting.h"
#  include "msgstream.h"
#  include "window_system.h"
#  include "../utils/config_store.h"
#  include "../input/input_injector.h"
#  include "../input/input_hook.h"
#  include "../input/input_driver.h"
#  include "../platform/window_system_interface.h"
#  include "../platform/input_injector_interface.h"
#  include "../platform/input_hook_interface.h"
#  include "../platform/input_driver_interface.h"
#  include <set>
#  include <queue>
#  include <string>
#  include "../functions/function.h"


enum {
    ///
    WM_APP_engineNotify = WM_APP + 110,
};


///
enum EngineNotify {
    EngineNotify_shellExecute,            ///
    EngineNotify_loadSetting,            ///
    EngineNotify_showDlg,                ///
    EngineNotify_helpMessage,            ///
    EngineNotify_setForegroundWindow,        ///
    EngineNotify_clearLog,            ///
};


///
class Engine : public StrExprSystem
{
private:
    enum {
        MAX_GENERATE_KEYBOARD_EVENTS_RECURSION_COUNT = 64, ///
        MAX_KEYMAP_PREFIX_HISTORY = 64, ///
    };

    typedef Keymaps::KeymapPtrList KeymapPtrList;    ///

    /// focus of a thread
    class FocusOfThread
    {
    public:
        DWORD m_threadId;                /// thread id
        HWND m_hwndFocus;                /** window that has focus on
                                                    the thread */
        std::string m_className;            /// class name of hwndFocus
        std::string m_titleName;            /// title name of hwndFocus
        bool m_isConsole;                /// is hwndFocus console ?
        KeymapPtrList m_keymaps;            /// keymaps

    public:
        ///
        FocusOfThread() : m_threadId(0), m_hwndFocus(nullptr), m_isConsole(false) { }
    };
    typedef std::map<DWORD /*ThreadId*/, FocusOfThread> FocusOfThreads;    ///

    typedef std::list<DWORD /*ThreadId*/> ThreadIds;    ///

    /// current status in generateKeyboardEvents
    class Current
    {
    public:
        const Keymap *m_keymap;            /// current keymap
        ModifiedKey m_mkey;        /// current processing key that user inputed
        /// index in currentFocusOfThread-&gt;keymaps
        Keymaps::KeymapPtrList::iterator m_i;

    public:
        ///
        bool isPressed() const {
            return m_mkey.m_modifier.isOn(Modifier::Type_Down);
        }
    };

    friend class FunctionParam;

#include "../functions/function_friends.h"

    /// part of keySeq
    enum Part {
        Part_all,                    ///
        Part_up,                    ///
        Part_down,                    ///
    };

    ///
    class EmacsEditKillLine
    {
        std::string m_buf;    /// previous kill-line contents

    public:
        bool m_doForceReset;    ///

    private:
        ///
        HGLOBAL makeNewKillLineBuf(const char *i_data, int *i_retval);

    public:
        ///
        void reset() {
            m_buf.resize(0);
        }
        /** EmacsEditKillLineFunc.
        clear the contents of the clopboard
        at that time, confirm if it is the result of the previous kill-line
        */
        void func();
        /// EmacsEditKillLinePred
        int pred();
    };

    /// window positon for &amp;WindowHMaximize, &amp;WindowVMaximize
    class WindowPosition
    {
    public:
        ///
        enum Mode {
            Mode_normal,                ///
            Mode_H,                    ///
            Mode_V,                    ///
            Mode_HV,                    ///
        };

    public:
        HWND m_hwnd;                ///
        RECT m_rc;                    ///
        Mode m_mode;                ///

    public:
        ///
        WindowPosition(HWND i_hwnd, const RECT &i_rc, Mode i_mode)
                : m_hwnd(i_hwnd), m_rc(i_rc), m_mode(i_mode) { }
    };
    typedef std::list<WindowPosition> WindowPositions;

    typedef std::list<HWND> WindowsWithAlpha; /// windows for &amp;WindowSetAlpha

    enum InterruptThreadReason {
        InterruptThreadReason_Terminate,
        InterruptThreadReason_Pause,
        InterruptThreadReason_Resume,
    };

    /* InputHandler removed (moved to Platform layer) */

private:
    CriticalSection m_cs;                /// criticalSection

    // setting
    HWND m_hwndAssocWindow;            /** associated window (we post
                                                    message to it) */
    Setting * volatile m_setting;            /// setting
    yamy::platform::IWindowSystem *m_windowSystem;            /// window system abstraction
    ConfigStore *m_configStore;            /// config store abstraction
    yamy::platform::IInputInjector *m_inputInjector;            /// input injector abstraction
    yamy::platform::IInputHook *m_inputHook;                /// input hook abstraction
    yamy::platform::IInputDriver *m_inputDriver;            /// input driver abstraction

    // engine thread state
    HANDLE m_threadHandle;
    unsigned m_threadId;
    std::deque<KEYBOARD_INPUT_DATA> *m_inputQueue;
    HANDLE m_queueMutex;
    // MSLLHOOKSTRUCT m_msllHookCurrent; // Moved to InputHook
    // bool m_buttonPressed; // Moved to InputHook
    // bool m_dragging; // Moved to InputHook
    // InputHandler m_keyboardHandler; // Moved to InputHook
    // InputHandler m_mouseHandler; // Moved to InputHook
    HANDLE m_readEvent;                /** reading from mayu device
                                                    has been completed */
    OVERLAPPED m_ol;                /** for async read/write of
                            mayu device */
    HANDLE m_hookPipe;                /// named pipe for &SetImeString
    HMODULE m_sts4mayu;                /// DLL module for ThumbSense
    HMODULE m_cts4mayu;                /// DLL module for ThumbSense
    bool volatile m_isLogMode;            /// is logging mode ?
    bool volatile m_isEnabled;            /// is enabled  ?
    bool volatile m_isSynchronizing;        /// is synchronizing ?
    HANDLE m_eSync;                /// event for synchronization
    int m_generateKeyboardEventsRecursionGuard;    /** guard against too many
                                                    recursion */

    // current key state
    Modifier m_currentLock;            /// current lock key's state
    int m_currentKeyPressCount;            /** how many keys are pressed
                                                    phisically ? */
    int m_currentKeyPressCountOnWin32;        /** how many keys are pressed
                                                    on win32 ? */
    Key *m_lastGeneratedKey;            /// last generated key
    Key *m_lastPressedKey[2];            /// last pressed key
    ModifiedKey m_oneShotKey;            /// one shot key
    unsigned int m_oneShotRepeatableRepeatCount; /// repeat count of one shot key
    bool m_isPrefix;                /// is prefix ?
    bool m_doesIgnoreModifierForPrefix;        /** does ignore modifier key
                                                    when prefixed ? */
    bool m_doesEditNextModifier;            /** does edit next user input
                                                    key's modifier ? */
    Modifier m_modifierForNextKey;        /** modifier for next key if
                                                    above is true */

    /** current keymaps.
        <dl>
        <dt>when &amp;OtherWindowClass
        <dd>currentKeymap becoms currentKeymaps[++ Current::i]
        <dt>when &amp;KeymapParent
        <dd>currentKeymap becoms currentKeyamp-&gt;parentKeymap
        <dt>other
        <dd>currentKeyamp becoms *Current::i
        </dl>
    */
    const Keymap * volatile m_currentKeymap;    /// current keymap
    FocusOfThreads /*volatile*/ m_focusOfThreads;    ///
    FocusOfThread * volatile m_currentFocusOfThread; ///
    FocusOfThread m_globalFocus;            ///
    HWND m_hwndFocus;                /// current focus window
    ThreadIds m_attachedThreadIds;    ///
    ThreadIds m_detachedThreadIds;    ///

    // for functions
    KeymapPtrList m_keymapPrefixHistory;        /// for &amp;KeymapPrevPrefix
    EmacsEditKillLine m_emacsEditKillLine;    /// for &amp;EmacsEditKillLine
    const ActionFunction *m_afShellExecute;    /// for &amp;ShellExecute

    WindowPositions m_windowPositions;        ///
    WindowsWithAlpha m_windowsWithAlpha;        ///

    std::string m_helpMessage;            /// for &amp;HelpMessage
    std::string m_helpTitle;                /// for &amp;HelpMessage
    int m_variable;                /// for &amp;Variable,
    ///  &amp;Repeat

public:
    tomsgstream &m_log;                /** log stream (output to log
                                                    dialog's edit) */

    /// Push input event to the queue (Thread Safe)
    void pushInputEvent(const KEYBOARD_INPUT_DATA &kid);

    /// Get current setting (Thread Safe - check for nullptr)
    const Setting *getSetting() const { return m_setting; }

public:
    // keyboardDetour and mouseDetour removed (moved to Platform layer)

private:
    // keyboardDetour and mouseDetour removed (moved to Platform layer)
    ///
    unsigned int injectInput(const KEYBOARD_INPUT_DATA *i_kid, const KBDLLHOOKSTRUCT *i_kidRaw);

private:
    /// keyboard handler thread
    static unsigned int WINAPI keyboardHandler(void *i_this);
    ///
    void keyboardHandler();

    /// check focus window
    void checkFocusWindow();
    /// is modifier pressed ?
    bool isPressed(Modifier::Type i_mt);
    /// fix modifier key
    bool fixModifierKey(ModifiedKey *io_mkey, Keymap::AssignMode *o_am);

    /// output to log
    void outputToLog(const Key *i_key, const ModifiedKey &i_mkey,
                     int i_debugLevel);

    /// genete modifier events
    void generateModifierEvents(const Modifier &i_mod);

    /// genete event
    void generateEvents(Current i_c, const Keymap *i_keymap, Key *i_event);

    /// generate keyboard event
    void generateKeyEvent(Key *i_key, bool i_doPress, bool i_isByAssign);
    ///
    void generateActionEvents(const Current &i_c, const Action *i_a,
                              bool i_doPress);
    ///
    void generateKeySeqEvents(const Current &i_c, const KeySeq *i_keySeq,
                              Part i_part);
    ///
    void generateKeyboardEvents(const Current &i_c);
    ///
    void beginGeneratingKeyboardEvents(const Current &i_c, bool i_isModifier);

    /// pop all pressed key on win32
    void keyboardResetOnWin32();

    /// get current modifiers
    Modifier getCurrentModifiers(Key *i_key, bool i_isPressed);

    /// describe bindings
    void describeBindings();

    /// update m_lastPressedKey
    void updateLastPressedKey(Key *i_key);

    /// set current keymap
    void setCurrentKeymap(const Keymap *i_keymap,
                          bool i_doesAddToHistory = false);
    /** open mayu device
        @return true if mayu device successfully is opened
    */
    bool open();

    /// close mayu device
    void close();

public:
    // BEGINING OF FUNCTION DEFINITION
    /// send a default key to Windows
// funcDefault removed (moved to Command_Default)
// funcKeymapParent removed (moved to Command_KeymapParent)
// funcKeymapWindow removed (moved to Command_KeymapWindow)
    /// use a corresponding key of the previous prefixed keymap
// funcKeymapPrevPrefix removed (moved to Command_KeymapPrevPrefix)
// funcOtherWindowClass removed (moved to Command_OtherWindowClass)
// funcPrefix removed (moved to Command_Prefix)
// funcKeymap removed (moved to Command_Keymap)
// funcSync removed (moved to Command_Sync)
// funcToggle removed (moved to Command_Toggle)
// funcEditNextModifier removed (moved to Command_EditNextModifier)
// funcVariable removed (moved to Command_Variable)
// funcRepeat removed (moved to Command_Repeat)
// funcUndefined removed (moved to Command_Undefined)
// funcIgnore removed (moved to Command_Ignore)
    /// post message
// funcPostMessage removed (moved to Command_PostMessage)
    /// ShellExecute
// funcShellExecute removed (moved to Command_ShellExecute)
    /// SetForegroundWindow
// funcSetForegroundWindow removed (moved to Command_SetForegroundWindow)
    /// load setting
// funcLoadSetting removed (moved to Command_LoadSetting)
// funcVK removed (moved to Command_VK)
// funcWait removed (moved to Command_Wait)
    /// investigate WM_COMMAND, WM_SYSCOMMAND
// funcInvestigateCommand removed (moved to Command_InvestigateCommand)
    /// show mayu dialog box
// funcMayuDialog removed (moved to Command_MayuDialog)
    /// describe bindings
// funcDescribeBindings removed (moved to Command_DescribeBindings)
    /// show help message
// funcHelpMessage removed (moved to Command_HelpMessage)
    /// show variable
// funcHelpVariable removed (moved to Command_HelpVariable)
    /// raise window
// funcWindowRaise removed (moved to Command_WindowRaise)
    /// lower window
// funcWindowLower removed (moved to Command_WindowLower)
    /// minimize window
// funcWindowMinimize removed (moved to Command_WindowMinimize)
    /// maximize window
// funcWindowMaximize removed (moved to Command_WindowMaximize)
    /// maximize window horizontally
// funcWindowHMaximize removed (moved to Command_WindowHMaximize)
    /// maximize window virtically
// funcWindowVMaximize removed (moved to Command_WindowVMaximize)
    /// maximize window virtically or horizontally
// funcWindowHVMaximize removed (moved to Command_WindowHVMaximize)
    /// move window
// funcWindowMove removed (moved to Command_WindowMove)
    /// move window to ...
// funcWindowMoveTo removed (moved to Command_WindowMoveTo)
    /// move window visibly
// funcWindowMoveVisibly removed (moved to Command_WindowMoveVisibly)
    /// move window to other monitor
// funcWindowMonitorTo removed (moved to Command_WindowMonitorTo)
    /// move window to other monitor
// funcWindowMonitor removed (moved to Command_WindowMonitor)
    ///
// funcWindowClingToLeft removed (moved to Command_WindowClingToLeft)
    ///
// funcWindowClingToRight removed (moved to Command_WindowClingToRight)
    ///
// funcWindowClingToTop removed (moved to Command_WindowClingToTop)
    ///
// funcWindowClingToBottom removed (moved to Command_WindowClingToBottom)
    /// close window
// funcWindowClose removed (moved to Command_WindowClose)
    /// toggle top-most flag of the window
// funcWindowToggleTopMost removed (moved to Command_WindowToggleTopMost)
    /// identify the window
// funcWindowIdentify removed (moved to Command_WindowIdentify)
    /// set alpha blending parameter to the window
// funcWindowSetAlpha removed (moved to Command_WindowSetAlpha)
    /// redraw the window
// funcWindowRedraw removed (moved to Command_WindowRedraw)
    /// resize window to
// funcWindowResizeTo removed (moved to Command_WindowResizeTo)
    /// move the mouse cursor
// funcMouseMove removed (moved to Command_MouseMove)
    /// send a mouse-wheel-message to Windows
// funcMouseWheel removed (moved to Command_MouseWheel)
    /// convert the contents of the Clipboard to upper case or lower case
// funcClipboardChangeCase removed (moved to Command_ClipboardChangeCase)
    /// convert the contents of the Clipboard to upper case
// funcClipboardUpcaseWord removed (moved to Command_ClipboardUpcaseWord)
    /// convert the contents of the Clipboard to lower case
// funcClipboardDowncaseWord removed (moved to Command_ClipboardDowncaseWord)
    /// set the contents of the Clipboard to the string
// funcClipboardCopy removed (moved to Command_ClipboardCopy)
    ///
// funcEmacsEditKillLinePred removed (moved to Command_EmacsEditKillLinePred)
    ///
// funcEmacsEditKillLineFunc removed (moved to Command_EmacsEditKillLineFunc)
    /// clear log
// funcLogClear removed (moved to Command_LogClear)
    /// recenter
// funcRecenter removed (moved to Command_Recenter)
    /// Direct SSTP
// funcDirectSSTP removed (moved to Command_DirectSSTP)
    /// PlugIn
// funcPlugIn removed (moved to Command_PlugIn)
    /// set IME open status
// funcSetImeStatus removed (moved to Command_SetImeStatus)
    /// set string to IME
// funcSetImeString removed (moved to Command_SetImeString)
    /// enter to mouse event hook mode
// funcMouseHook removed (moved to Command_MouseHook)
    /// cancel prefix
// funcCancelPrefix removed (moved to Command_CancelPrefix)

    // END OF FUNCTION DEFINITION
#include "function_friends.h"

public:
    // Helper functions for commands
    static bool getSuitableWindow(FunctionParam *i_param, HWND *o_hwnd);
    static bool getSuitableMdiWindow(yamy::platform::IWindowSystem *ws, FunctionParam *i_param, HWND *o_hwnd,
                                     TargetWindowType *io_twt,
                                     RECT *o_rcWindow = nullptr, RECT *o_rcParent = nullptr);

    ///
    Engine(tomsgstream &i_log, yamy::platform::IWindowSystem *i_windowSystem, ConfigStore *i_configStore, yamy::platform::IInputInjector *i_inputInjector, yamy::platform::IInputHook *i_inputHook, yamy::platform::IInputDriver *i_inputDriver);
    ///
    ~Engine();

    /// start/stop keyboard handler thread
    void start();
    ///
    void stop();

    /// pause keyboard handler thread and close device
    bool pause();

    /// resume keyboard handler thread and re-open device
    bool resume();

    /// do some procedure before quit which must be done synchronously
    /// (i.e. not on WM_QUIT)
    bool prepairQuit();

    /// logging mode
    void enableLogMode(bool i_isLogMode = true) {
        m_isLogMode = i_isLogMode;
    }
    ///
    void disableLogMode() {
        m_isLogMode = false;
    }

    /// enable/disable engine
    void enable(bool i_isEnabled = true) {
        m_isEnabled = i_isEnabled;
    }
    ///
    void disable() {
        m_isEnabled = false;
    }
    ///
    bool getIsEnabled() const {
        return m_isEnabled;
    }

    /// associated window
    void setAssociatedWndow(HWND i_hwnd) {
        m_hwndAssocWindow = i_hwnd;
    }

    /// associated window
    HWND getAssociatedWndow() const {
        return m_hwndAssocWindow;
    }

    /// setting
    bool setSetting(Setting *i_setting);

    /// focus
    bool setFocus(HWND i_hwndFocus, DWORD i_threadId,
                  const std::string &i_className,
                  const std::string &i_titleName, bool i_isConsole);

    /// lock state
    bool setLockState(bool i_isNumLockToggled, bool i_isCapsLockToggled,
                      bool i_isScrollLockToggled, bool i_isKanaLockToggled,
                      bool i_isImeLockToggled, bool i_isImeCompToggled);

    /// show
    void checkShow(HWND i_hwnd);
    bool setShow(bool i_isMaximized, bool i_isMinimized, bool i_isMDI);

    /// sync
    bool syncNotify();

    /// thread attach notify
    bool threadAttachNotify(DWORD i_threadId);

    /// thread detach notify
    bool threadDetachNotify(DWORD i_threadId);

    /// shell execute
    void shellExecute();

    /// get help message
    void getHelpMessages(std::string *o_helpMessage, std::string *o_helpTitle);

    /// command notify
    template <typename WPARAM_T, typename LPARAM_T>
    void commandNotify(HWND i_hwnd, UINT i_message, WPARAM_T i_wParam,
                       LPARAM_T i_lParam)
    {
        Acquire b(&m_log, 0);
        HWND hf = m_hwndFocus;
        if (!hf)
            return;

        if (GetWindowThreadProcessId(hf, nullptr) ==
                GetWindowThreadProcessId(m_hwndAssocWindow, nullptr))
            return;    // inhibit the investigation of MADO TSUKAI NO YUUTSU

        const char *target = nullptr;
        int number_target = 0;

        if (i_hwnd == hf)
            target = "ToItself";
        else if (i_hwnd == GetParent(hf))
            target = "ToParentWindow";
        else {
            // Function::toMainWindow
            HWND h = hf;
            while (true) {
                HWND p = GetParent(h);
                if (!p)
                    break;
                h = p;
            }
            if (i_hwnd == h)
                target = "ToMainWindow";
            else {
                // Function::toOverlappedWindow
                HWND h = hf;
                while (h) {
#ifdef MAYU64
                    LONG_PTR style = GetWindowLongPtr(h, GWL_STYLE);
#else
                    LONG style = GetWindowLong(h, GWL_STYLE);
#endif
                    if ((style & WS_CHILD) == 0)
                        break;
                    h = GetParent(h);
                }
                if (i_hwnd == h)
                    target = "ToOverlappedWindow";
                else {
                    // number
                    HWND h = hf;
                    for (number_target = 0; h; number_target ++, h = GetParent(h))
                        if (i_hwnd == h)
                            break;
                    return;
                }
            }
        }

        m_log << "&PostMessage(";
        if (target)
            m_log << target;
        else
            m_log << number_target;
        m_log << ", " << i_message
        << ", 0x" << std::hex << i_wParam
        << ", 0x" << i_lParam << ") # hwnd = "
        << reinterpret_cast<ULONG_PTR>(i_hwnd) << ", "
        << "message = " << std::dec;
        if (i_message == WM_COMMAND)
            m_log << "WM_COMMAND, ";
        else if (i_message == WM_SYSCOMMAND)
            m_log << "WM_SYSCOMMAND, ";
        else
            m_log << i_message << ", ";
        m_log << "wNotifyCode = " << HIWORD(i_wParam) << ", "
        << "wID = " << LOWORD(i_wParam) << ", "
        << "hwndCtrl = 0x" << std::hex << i_lParam << std::dec << std::endl;
    }

    /// get current window class name
    const std::string &getCurrentWindowClassName() const {
        return m_currentFocusOfThread->m_className;
    }

    /// get current window title name
    const std::string &getCurrentWindowTitleName() const {
        return m_currentFocusOfThread->m_titleName;
    }

    // StrExprSystem overrides
    std::string getClipboardText() const override;
    std::string getStrExprWindowClassName() const override;
    std::string getStrExprWindowTitleName() const override;
};

///
class FunctionParam
{
public:
    bool m_isPressed;                /// is key pressed ?
    HWND m_hwnd;                    ///
    Engine::Current m_c;                /// new context
    bool m_doesNeedEndl;                /// need endl ?
    const ActionFunction *m_af;            ///
};


#endif // !_ENGINE_H
