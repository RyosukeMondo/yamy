//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_lifecycle.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>
#include <process.h>


Engine::Engine(tomsgstream &i_log, WindowSystem *i_windowSystem, ConfigStore *i_configStore, InputInjector *i_inputInjector, InputHook *i_inputHook, InputDriver *i_inputDriver)
        : m_hwndAssocWindow(NULL),
        m_setting(NULL),
        m_windowSystem(i_windowSystem),
        m_configStore(i_configStore),
        m_inputInjector(i_inputInjector),
        m_inputHook(i_inputHook),
        m_inputDriver(i_inputDriver),
        m_inputQueue(NULL),
        m_readEvent(NULL),
        m_queueMutex(NULL),
        m_sts4mayu(NULL),
        m_cts4mayu(NULL),
        m_isLogMode(false),
        m_isEnabled(true),
        m_isSynchronizing(false),
        m_eSync(NULL),
        m_generateKeyboardEventsRecursionGuard(0),
        m_currentKeyPressCount(0),
        m_currentKeyPressCountOnWin32(0),
        m_lastGeneratedKey(NULL),
        m_oneShotRepeatableRepeatCount(0),
        m_isPrefix(false),
        m_currentKeymap(NULL),
        m_currentFocusOfThread(NULL),
        m_hwndFocus(NULL),
        m_afShellExecute(NULL),
        m_variable(0),
        m_log(i_log) {
    BOOL (WINAPI *pChangeWindowMessageFilter)(UINT, DWORD) =
        reinterpret_cast<BOOL (WINAPI*)(UINT, DWORD)>(GetProcAddress(GetModuleHandle(_T("user32.dll")), "ChangeWindowMessageFilter"));

    if(pChangeWindowMessageFilter != NULL) {
        pChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
    }

    for (size_t i = 0; i < NUMBER_OF(m_lastPressedKey); ++ i)
        m_lastPressedKey[i] = NULL;

    // set default lock state
    for (int i = 0; i < Modifier::Type_end; ++ i)
        m_currentLock.dontcare(static_cast<Modifier::Type>(i));
    for (int i = Modifier::Type_Lock0; i <= Modifier::Type_Lock9; ++ i)
        m_currentLock.release(static_cast<Modifier::Type>(i));

    // create event for sync
    CHECK_TRUE( m_eSync = CreateEvent(NULL, FALSE, FALSE, NULL) );
    // create named pipe for &SetImeString
    m_hookPipe = CreateNamedPipe(addSessionId(HOOK_PIPE_NAME).c_str(),
                                 PIPE_ACCESS_OUTBOUND,
                                 PIPE_TYPE_BYTE, 1,
                                 0, 0, 0, NULL);
    StrExprArg::setSystem(this);
}


Engine::~Engine() {
    CHECK_TRUE( CloseHandle(m_eSync) );

    // destroy named pipe for &SetImeString
    if (m_hookPipe && m_hookPipe != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(m_hookPipe);
        CHECK_TRUE( CloseHandle(m_hookPipe) );
    }
}


// start keyboard handler thread
void Engine::start() {
    m_inputHook->start(this);

    CHECK_TRUE( m_inputQueue = new std::deque<KEYBOARD_INPUT_DATA> );
    CHECK_TRUE( m_queueMutex = CreateMutex(NULL, FALSE, NULL) );
    CHECK_TRUE( m_readEvent = CreateEvent(NULL, TRUE, FALSE, NULL) );
    m_ol.Offset = 0;
    m_ol.OffsetHigh = 0;
    m_ol.hEvent = m_readEvent;

    m_inputDriver->open(m_readEvent);

    CHECK_TRUE( m_threadHandle = (HANDLE)_beginthreadex(NULL, 0, keyboardHandler, this, 0, &m_threadId) );
}


// stop keyboard handler thread
void Engine::stop() {
    m_inputHook->stop();
    m_inputDriver->close();

    WaitForSingleObject(m_queueMutex, INFINITE);
    delete m_inputQueue;
    m_inputQueue = NULL;
    SetEvent(m_readEvent);
    ReleaseMutex(m_queueMutex);

    WaitForSingleObject(m_threadHandle, 2000);
    CHECK_TRUE( CloseHandle(m_threadHandle) );
    m_threadHandle = NULL;

    CHECK_TRUE( CloseHandle(m_readEvent) );
    m_readEvent = NULL;

    for (ThreadIds::iterator i = m_attachedThreadIds.begin();
         i != m_attachedThreadIds.end(); i++) {
         PostThreadMessage(*i, WM_NULL, 0, 0);
    }
}


bool Engine::prepairQuit() {
    // terminate and unload DLL for ThumbSense support if loaded
    m_inputDriver->manageExtension(_T("sts4mayu.dll"), _T("SynCOM.dll"),
                  false, (void**)&m_sts4mayu);
    m_inputDriver->manageExtension(_T("cts4mayu.dll"), _T("TouchPad.dll"),
                  false, (void**)&m_cts4mayu);
    return true;
}


// sync
bool Engine::syncNotify() {
    Acquire a(&m_cs);
    if (!m_isSynchronizing)
        return false;
    CHECK_TRUE( SetEvent(m_eSync) );
    return true;
}


// update m_lastPressedKey
void Engine::updateLastPressedKey(Key *i_key)
{
    m_lastPressedKey[1] = m_lastPressedKey[0];
    m_lastPressedKey[0] = i_key;
}


// set current keymap
void Engine::setCurrentKeymap(const Keymap *i_keymap, bool i_doesAddToHistory)
{
    if (i_doesAddToHistory) {
        m_keymapPrefixHistory.push_back(const_cast<Keymap *>(m_currentKeymap));
        if (MAX_KEYMAP_PREFIX_HISTORY < m_keymapPrefixHistory.size())
            m_keymapPrefixHistory.pop_front();
    } else
        m_keymapPrefixHistory.clear();
    m_currentKeymap = i_keymap;
}


void Engine::pushInputEvent(const KEYBOARD_INPUT_DATA &kid)
{
    WaitForSingleObject(m_queueMutex, INFINITE);
    if (m_inputQueue) {
        m_inputQueue->push_back(kid);
        SetEvent(m_readEvent);
    }
    ReleaseMutex(m_queueMutex);
}

