#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine.h


#ifndef _ENGINE_H
#  define _ENGINE_H

#  include "multithread.h"
#  include "setting.h"
#  include "msgstream.h"
#  include "../platform/window_system_interface.h"
#  include "../platform/input_injector_interface.h"
#  include "../platform/input_hook_interface.h"
#  include "../platform/input_driver_interface.h"
#  include "../utils/config_store.h"
#  include "../settings/config_manager.h"
#  include "../ipc_messages.h"
#  include "engine_state.h"
#  include "../platform/ipc_defs.h"
#  include "../platform/ipc_channel_interface.h"
#  include <set>
#  include <queue>
#  include "../audio/sound_manager.h"
#  include "../functions/function.h"
#  include "../input/input_event.h" // For KEYBOARD_INPUT_DATA (legacy)
#  include "../input/modifier_state.h" // For ModifierState tracking
#  include "../platform/types.h" // For KeyEvent
#  include "../platform/message_constants.h"
#  include "engine_event_processor.h" // For unified 3-layer event processing
#  include "compiled_rule.h" // For CompiledRule
#  include <functional>
#  include <gsl/gsl>

enum {
    ///
    WM_APP_engineNotify = yamy::platform::MSG_APP_ENGINE_NOTIFY,
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


/// Callback type for configuration switch notifications
using ConfigSwitchCallback = std::function<void(bool success, const std::string& configPath)>;

///
class Engine : public StrExprSystem
{
private:
    enum {
        MAX_GENERATE_KEYBOARD_EVENTS_RECURSION_COUNT = 64, ///
        MAX_KEYMAP_PREFIX_HISTORY = 64, ///
    };

    typedef Keymaps::KeymapPtrList KeymapPtrList;    ///

    /// current status in generateKeyboardEvents
    class Current
    {
    public:
        const Keymap *m_keymap;            /// current keymap
        ModifiedKey m_mkey;        /// current processing key that user inputed
        /// index in keymap list for iteration
        Keymaps::KeymapPtrList::iterator m_i;
        /// Original evdev code for EventProcessor (0 if not available)
        uint16_t m_evdev_code;

    public:
        ///
        Current() : m_keymap(nullptr), m_evdev_code(0) { }
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
        void* makeNewKillLineBuf(const char *i_data, int *i_retval);

    public:
        ///
        void reset() {
            m_buf.resize(0);
        }
        /** EmacsEditKillLineFunc.
        clear the contents of the clopboard
        at that time, confirm if it is the result of the previous kill-line
        */
        void func(yamy::platform::IWindowSystem* ws);
        /// EmacsEditKillLinePred
        int pred(yamy::platform::IWindowSystem* ws);
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
        yamy::platform::WindowHandle m_hwnd;                ///
        yamy::platform::Rect m_rc;                    ///
        Mode m_mode;                ///

    public:
        ///
        WindowPosition(yamy::platform::WindowHandle i_hwnd, const yamy::platform::Rect &i_rc, Mode i_mode)
                : m_hwnd(i_hwnd), m_rc(i_rc), m_mode(i_mode) { }
    };
    typedef std::list<WindowPosition> WindowPositions;

    typedef std::list<yamy::platform::WindowHandle> WindowsWithAlpha; /// windows for &amp;WindowSetAlpha

    enum InterruptThreadReason {
        InterruptThreadReason_Terminate,
        InterruptThreadReason_Pause,
        InterruptThreadReason_Resume,
    };

private:
    CriticalSection m_cs;                /// criticalSection

    // setting
    yamy::platform::WindowHandle m_hwndAssocWindow;            /** associated window (we post
                                                    message to it) */
    Setting * volatile m_setting;            /// setting
    std::string m_currentConfigPath;        /// currently loaded config path (guard against reload loops)
    yamy::platform::IWindowSystem *m_windowSystem;            /// window system abstraction
    ConfigStore *m_configStore;            /// config store abstraction
    ConfigSwitchCallback m_configSwitchCallback; /// config switch notification callback
    yamy::platform::IInputInjector *m_inputInjector;            /// input injector abstraction
    yamy::platform::IInputHook *m_inputHook;                /// input hook abstraction
    yamy::platform::IInputDriver *m_inputDriver;            /// input driver abstraction
    std::unique_ptr<yamy::platform::IIPCChannel> m_ipcChannel; /// IPC channel for UI communication
#if defined(QT_CORE_LIB)
    std::unique_ptr<yamy::audio::SoundManager> m_soundManager;
#endif

    // engine thread state
    yamy::platform::ThreadHandle m_threadHandle;
    unsigned m_threadId;
    std::deque<yamy::platform::KeyEvent> *m_inputQueue;
    yamy::platform::MutexHandle m_queueMutex;

    yamy::platform::EventHandle m_readEvent;                /** reading from mayu device
                                                    has been completed */
    yamy::platform::OverlappedHandle m_ol;                /** for async read/write of
                            mayu device */
    yamy::platform::EventHandle m_hookPipe;                /// named pipe for &SetImeString
    yamy::platform::ModuleHandle m_sts4mayu;                /// DLL module for ThumbSense
    yamy::platform::ModuleHandle m_cts4mayu;                /// DLL module for ThumbSense
    bool volatile m_isLogMode;            /// is logging mode ?
    bool volatile m_isEnabled;            /// is enabled  ?
    bool volatile m_isInvestigateMode;    /// is investigate mode enabled?
    bool volatile m_isSynchronizing;        /// is synchronizing ?
    yamy::platform::EventHandle m_eSync;                /// event for synchronization
    int m_generateKeyboardEventsRecursionGuard;    /** guard against too many
                                                    recursion */
    yamy::EngineState m_state;              /// current engine state

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
        <dd>currentKeymap becoms *Current::i
        </dl>
    */
    const Keymap * volatile m_currentKeymap;    /// current keymap
    const Keymap * m_globalKeymap;    /// global keymap for simplified single-keymap model

    // for functions
    KeymapPtrList m_keymapPrefixHistory;        /// for &amp;KeymapPrevPrefix
    EmacsEditKillLine m_emacsEditKillLine;    /// for &amp;EmacsEditKillLine
    const ActionFunction *m_afShellExecute;    /// for &amp;ShellExecute

    WindowPositions m_windowPositions;        ///
    WindowsWithAlpha m_windowsWithAlpha;        ///

    std::string m_helpMessage;            /// for &amp;HelpMessage
    std::string m_helpTitle;                /// for &amp;HelpMessage
    int m_variable;                /// for &amp;Variable,
    std::chrono::steady_clock::time_point m_lastFocusChangedTime; /// for debouncing focus change notifications
    ///  &amp;Repeat
    
    yamy::platform::ThreadHandle m_perfThreadHandle; /// thread for performance metrics
    bool volatile m_isPerfThreadRunning;     /// flag to control performance thread

    // Event processing
    std::unique_ptr<yamy::EventProcessor> m_eventProcessor; /// Unified 3-layer event processor
    yamy::input::ModifierState m_modifierState; /// Modal and hardware modifier state tracking

    /// Keymap entry for virtual modifier/lock-based key matching
    struct KeymapEntry {
        uint32_t required_mods[8];   ///< Bitmask of required M00-MFF modifiers
        uint32_t required_locks[8];  ///< Bitmask of required L00-LFF locks
        uint16_t input_key;          ///< Input YAMY scan code
        uint16_t output_key;         ///< Output YAMY scan code
        uint8_t specificity;         ///< Precomputed: popcount(mods) + popcount(locks)

        KeymapEntry() : input_key(0), output_key(0), specificity(0) {
            std::memset(required_mods, 0, sizeof(required_mods));
            std::memset(required_locks, 0, sizeof(required_locks));
        }
    };

    std::vector<KeymapEntry> m_virtualKeymap;  ///< Sorted by specificity DESC for virtual key system

public:
    tomsgstream &m_log;                /** log stream (output to log
                                                    dialog's edit) */

    /**
     * @brief Push input event to the queue (Thread Safe)
     * @param event The keyboard event to push
     * @pre event.scanCode <= 0xFFFF (valid scan code range)
     */
    void pushInputEvent(const yamy::platform::KeyEvent &event);

    /// Convert KeyEvent to KEYBOARD_INPUT_DATA (for legacy code)
    static KEYBOARD_INPUT_DATA keyEventToKID(const yamy::platform::KeyEvent &event);

    /// Get current setting (Thread Safe - check for nullptr)
    const Setting *getSetting() const { return m_setting; }

    /// Get window system interface
    yamy::platform::IWindowSystem* getWindowSystem() const { return m_windowSystem; }

public:
    // keyboardDetour and mouseDetour removed (moved to Platform layer)

private:
    // keyboardDetour and mouseDetour removed (moved to Platform layer)
    ///
    unsigned int injectInput(const KEYBOARD_INPUT_DATA *i_kid, const void *i_kidRaw);

private:
    /// keyboard handler thread (static entry point)
    static void* keyboardHandler(void *i_this);
    /// keyboard handler thread (instance method)
    void keyboardHandler();

    /// performance metrics thread (static entry point)
    static void* perfMetricsHandler(void *i_this);
    /// performance metrics thread (instance method)
    void perfMetricsHandler();

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

    /// Check if any virtual modifiers (M00-MFF) are active
    bool hasActiveVirtualModifiers() const;

    /// Build ModifiedKey with physical key + active modifiers (before substitution)
    ModifiedKey buildPhysicalModifiedKey(const Current& i_c) const;

    /// Set the current engine state and log the transition
    void setState(yamy::EngineState i_newState);

    /// Send a notification message to the GUI via the IPC channel (string data)
    void notifyGUI(yamy::MessageType i_type, const std::string &i_data = "");

    /// Send a notification message to the GUI via the IPC channel (binary data)
    void notifyGUI(yamy::MessageType i_type, const void* i_data, size_t i_size);

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

    /// Build substitution table from Keyboard::Substitutes
    /// @param keyboard Reference to Keyboard object with substitution mappings
    void buildSubstitutionTable(const Keyboard &keyboard);

    /// Lookup keymap entry with modifier/lock matching and specificity priority
    /// @param key Input YAMY scan code to match
    /// @param mods Active modifier state (M00-MFF)
    /// @return Output YAMY scan code, or 0 if no match found
    uint16_t lookupKeymap(uint16_t key, const yamy::input::ModifierState& mods) const;

    /// Sort keymap entries by specificity (highest first) after loading
    void sortKeymapBySpecificity();

    /// Add a keymap entry with automatic specificity calculation
    /// @param input_key Input YAMY scan code
    /// @param output_key Output YAMY scan code
    /// @param required_mods Required modifier bitmask array (8 x uint32_t)
    /// @param required_locks Required lock bitmask array (8 x uint32_t)
    void addKeymapEntry(uint16_t input_key, uint16_t output_key,
                        const uint32_t required_mods[8],
                        const uint32_t required_locks[8]);

    /// Compile a legacy Substitute rule into the new format for the O(1) lookup table
    std::vector<yamy::engine::CompiledRule> compileSubstitute(const Keyboard::Substitute& sub);

    /** open mayu device
        @return true if mayu device successfully is opened
    */
    bool open();

    /// close mayu device
    void close();

public:
    // BEGINING OF FUNCTION DEFINITION
    // ... (unchanged)
    // END OF FUNCTION DEFINITION
#include "function_friends.h"

public:
    // Helper functions for commands
    static bool getSuitableWindow(yamy::platform::IWindowSystem *ws, FunctionParam *i_param, yamy::platform::WindowHandle *o_hwnd);
    static bool getSuitableMdiWindow(yamy::platform::IWindowSystem *ws, FunctionParam *i_param, yamy::platform::WindowHandle *o_hwnd,
                                     TargetWindowType *io_twt,
                                     yamy::platform::Rect *o_rcWindow = nullptr, yamy::platform::Rect *o_rcParent = nullptr);

    /**
     * @brief Construct a new Engine object
     * @param i_log Reference to the message stream for logging
     * @param i_windowSystem Pointer to the window system interface
     * @param i_configStore Pointer to the configuration store
     * @param i_inputInjector Pointer to the input injector interface
     * @param i_inputHook Pointer to the input hook interface
     * @param i_inputDriver Pointer to the input driver interface
     * @pre i_windowSystem != nullptr
     * @pre i_configStore != nullptr
     * @pre i_inputInjector != nullptr
     * @pre i_inputHook != nullptr
     * @pre i_inputDriver != nullptr
     */
    Engine(tomsgstream &i_log, yamy::platform::IWindowSystem *i_windowSystem, ConfigStore *i_configStore, yamy::platform::IInputInjector *i_inputInjector, yamy::platform::IInputHook *i_inputHook, yamy::platform::IInputDriver *i_inputDriver);

    /// Destroy the Engine object
    ~Engine();

    /// start/stop keyboard handler thread
    void start();
    ///
    void stop();

    /// pause keyboard handler thread and close device
    bool pause();

    /// resume keyboard handler thread and re-open device
    bool resume();

    /// Initialize IPC channel (must be called AFTER Qt event loop starts)
    /// This is separated from constructor to avoid QSocketNotifier threading issues
    void initializeIPC();

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

    /**
     * @brief Set associated window for message posting
     * @param i_hwnd Window handle to associate
     * @pre i_hwnd != nullptr
     */
    void setAssociatedWndow(yamy::platform::WindowHandle i_hwnd) {
        m_hwndAssocWindow = i_hwnd;
    }

    /// associated window
    yamy::platform::WindowHandle getAssociatedWndow() const {
        return m_hwndAssocWindow;
    }

    /**
     * @brief Set the active setting configuration
     * @param i_setting Pointer to the setting object
     * @return true if setting was applied successfully, false otherwise
     * @pre i_setting != nullptr
     */
    bool setSetting(Setting *i_setting);

    /**
     * @brief Switch to a different configuration file
     * @param configPath Path to the .mayu configuration file
     * @return true if switch was successful, false on parse errors
     * @pre !configPath.empty()
     */
    bool switchConfiguration(const std::string& configPath);

    /// Set callback for configuration switch notifications
    void setConfigSwitchCallback(ConfigSwitchCallback callback) {
        m_configSwitchCallback = callback;
    }


    /// lock state
    bool setLockState(bool i_isNumLockToggled, bool i_isCapsLockToggled,
                      bool i_isScrollLockToggled, bool i_isKanaLockToggled,
                      bool i_isImeLockToggled, bool i_isImeCompToggled);

    /**
     * @brief Check if a window should be shown
     * @param i_hwnd Window handle to check
     * @pre i_hwnd != nullptr
     */
    void checkShow(yamy::platform::WindowHandle i_hwnd);

    /// Set show state flags
    bool setShow(bool i_isMaximized, bool i_isMinimized, bool i_isMDI);

    /// sync
    bool syncNotify();


    /// shell execute
    void shellExecute();

    /**
     * @brief Get help messages for display
     * @param o_helpMessage Output pointer for help message text
     * @param o_helpTitle Output pointer for help title text
     * @pre o_helpMessage != nullptr
     * @pre o_helpTitle != nullptr
     */
    void getHelpMessages(std::string *o_helpMessage, std::string *o_helpTitle);

    /// command notify
    template <typename WParamT, typename LParamT>
    void commandNotify(yamy::platform::WindowHandle i_hwnd, unsigned int i_message, WParamT i_wParam,
                       LParamT i_lParam)
    {
        // Simplified: no focus tracking, use provided window handle
        Acquire b(&m_log, 0);
        if (!i_hwnd)
            return;
        m_log << "Command Notify (logging disabled during refactor)" << std::endl;
        return;
    }


    yamy::EngineState getState() const {
        return m_state;
    }

#if defined(QT_CORE_LIB)
    /// Play a notification sound
    void playSound(yamy::audio::NotificationType type);
#endif

    /**
     * @brief Handle incoming IPC messages from the Qt dialog
     *
     * Processes IPC messages received from the investigate dialog via IPCChannelQt.
     * Supports CmdInvestigateWindow requests by querying keymap status and sending
     * RspInvestigateWindow responses.
     *
     * @param message IPC message containing command type and data
     *
     * @note This method is called via Qt signal/slot when messages arrive
     * @note Connected in engine_lifecycle.cpp constructor
     *
     * @see queryKeymapForWindow()
     */
    void handleIpcMessage(const yamy::ipc::Message& message);

    // StrExprSystem overrides
    std::string getClipboardText() const override;
    std::string getStrExprWindowClassName() const override;
    std::string getStrExprWindowTitleName() const override;

    /**
     * @brief Keymap status information for investigate dialog
     *
     * Contains detailed information about which keymap is active for a window
     * and why it was selected. Used by the investigate dialog to display
     * keymap matching details.
     */
    struct KeymapStatus {
        std::string keymapName;         ///< Name of the matched keymap (empty if default/global)
        std::string matchedClassRegex;  ///< Window class regex that matched (from .mayu)
        std::string matchedTitleRegex;  ///< Window title regex that matched (from .mayu)
        std::string activeModifiers;    ///< Currently active modifiers as string (e.g., "Ctrl+Shift")
        bool isDefault;                 ///< True if using default/global keymap (no specific match)
    };

    /**
     * @brief Query keymap status for a given window
     *
     * Determines which keymap would be active for the specified window by
     * matching window class and title against configured keymap rules.
     * Returns detailed status including matched regex patterns and active modifiers.
     *
     * @param hwnd Window handle to query keymap for
     * @param className Window class name (from getClassName())
     * @param titleName Window title name (from getWindowText())
     * @return KeymapStatus structure with matched keymap information
     *
     * @note This method does not change engine state, only queries current keymap rules
     * @note Used by investigate dialog to display keymap debugging information
     * @pre hwnd != nullptr
     */
    KeymapStatus queryKeymapForWindow(yamy::platform::WindowHandle hwnd,
                                      const std::string& className,
                                      const std::string& titleName) const;
};

///
class FunctionParam
{
public:
    bool m_isPressed;                /// is key pressed ?
    yamy::platform::WindowHandle m_hwnd;                    ///
    Engine::Current m_c;                /// new context
    bool m_doesNeedEndl;                /// need endl ?
    const ActionFunction *m_af;            ///
};


#endif // !_ENGINE_H
