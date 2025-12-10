//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_lifecycle.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#ifdef _WIN32
#include "windowstool.h"
#endif
#include "../platform/message_constants.h"
#include "../platform/sync.h"

#include <iomanip>
#ifdef _WIN32
#include <process.h>
#endif
#include <string>


Engine::Engine(tomsgstream &i_log, yamy::platform::IWindowSystem *i_windowSystem, ConfigStore *i_configStore, yamy::platform::IInputInjector *i_inputInjector, yamy::platform::IInputHook *i_inputHook, yamy::platform::IInputDriver *i_inputDriver)
        : m_hwndAssocWindow(nullptr),
        m_setting(nullptr),
        m_windowSystem(i_windowSystem),
        m_configStore(i_configStore),
        m_inputInjector(i_inputInjector),
        m_inputHook(i_inputHook),
        m_inputDriver(i_inputDriver),
        m_inputQueue(nullptr),
        m_queueMutex(nullptr),
        m_readEvent(nullptr),
        m_sts4mayu(nullptr),
        m_cts4mayu(nullptr),
        m_isLogMode(false),
        m_isEnabled(true),
        m_isSynchronizing(false),
        m_eSync(nullptr),
        m_generateKeyboardEventsRecursionGuard(0),
        m_currentKeyPressCount(0),
        m_currentKeyPressCountOnWin32(0),
        m_lastGeneratedKey(nullptr),
        m_oneShotRepeatableRepeatCount(0),
        m_isPrefix(false),
        m_currentKeymap(nullptr),
        m_currentFocusOfThread(nullptr),
        m_hwndFocus(nullptr),
        m_afShellExecute(nullptr),
        m_variable(0),
        m_log(i_log) {
    // Enable receiving WM_COPYDATA from lower integrity processes
    m_windowSystem->changeMessageFilter(yamy::platform::MSG_COPYDATA,
                                       yamy::platform::MSGFLT_ADD);

    for (size_t i = 0; i < NUMBER_OF(m_lastPressedKey); ++ i)
        m_lastPressedKey[i] = nullptr;

    // set default lock state
    for (int i = 0; i < Modifier::Type_end; ++ i)
        m_currentLock.dontcare(static_cast<Modifier::Type>(i));
    for (int i = Modifier::Type_Lock0; i <= Modifier::Type_Lock9; ++ i)
        m_currentLock.release(static_cast<Modifier::Type>(i));

    // create event for sync
#ifdef _WIN32
    CHECK_TRUE( m_eSync = CreateEvent(nullptr, FALSE, FALSE, nullptr) );
#endif
    // create named pipe for &SetImeString
#ifdef _WIN32
    m_hookPipe = CreateNamedPipe(addSessionId(HOOK_PIPE_NAME).c_str(),
                                 PIPE_ACCESS_OUTBOUND,
                                 PIPE_TYPE_BYTE, 1,
                                 0, 0, 0, nullptr);
#endif
    StrExprArg::setSystem(this);
}


Engine::~Engine() {
#ifdef _WIN32
    CHECK_TRUE( CloseHandle(m_eSync) );

    // destroy named pipe for &SetImeString
    if (m_hookPipe && m_hookPipe != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(m_hookPipe);
        CHECK_TRUE( CloseHandle(m_hookPipe) );
    }
#endif
}


// start keyboard handler thread
void Engine::start() {
    m_inputHook->install(
        [this](const yamy::platform::KeyEvent& event) {
            // Pass KeyEvent directly to the queue
            this->pushInputEvent(event);
            return true;
        },
        [this](const yamy::platform::MouseEvent& e) {
            // Mouse event handler (currently unused)
            return true;
        }
    );

    CHECK_TRUE( m_inputQueue = new std::deque<yamy::platform::KeyEvent> );
#ifdef _WIN32
    CHECK_TRUE( m_queueMutex = CreateMutex(nullptr, FALSE, nullptr) );
    CHECK_TRUE( m_readEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr) );
#endif
#ifdef _WIN32
    OVERLAPPED* pOl = reinterpret_cast<OVERLAPPED*>(m_ol);
    pOl->Offset = 0;
    pOl->OffsetHigh = 0;
    pOl->hEvent = m_readEvent;
#endif

    m_inputDriver->open(m_readEvent);

#ifdef _WIN32
    CHECK_TRUE( m_threadHandle = reinterpret_cast<yamy::platform::ThreadHandle>(_beginthreadex(nullptr, 0, keyboardHandler, this, 0, &m_threadId)) );
#endif
}


// stop keyboard handler thread
void Engine::stop() {
    m_inputHook->uninstall();
    m_inputDriver->close();

#ifdef _WIN32
    yamy::platform::waitForObject(m_queueMutex, yamy::platform::WAIT_INFINITE);
    delete m_inputQueue;
    m_inputQueue = nullptr;
    SetEvent(m_readEvent);
    ReleaseMutex(m_queueMutex);

    yamy::platform::waitForObject(m_threadHandle, 2000);
    CHECK_TRUE( CloseHandle(m_threadHandle) );
    m_threadHandle = nullptr;

    CHECK_TRUE( CloseHandle(m_readEvent) );
    m_readEvent = nullptr;
#endif

    for (ThreadIds::iterator i = m_attachedThreadIds.begin();
         i != m_attachedThreadIds.end(); i++) {
         PostThreadMessage(*i, WM_NULL, 0, 0);
    }
}


bool Engine::prepairQuit() {
    // terminate and unload DLL for ThumbSense support if loaded
    m_inputDriver->manageExtension("sts4mayu.dll", "SynCOM.dll",
                  false, (void**)&m_sts4mayu);
    m_inputDriver->manageExtension("cts4mayu.dll", "TouchPad.dll",
                  false, (void**)&m_cts4mayu);
    return true;
}


// sync
bool Engine::syncNotify() {
    Acquire a(&m_cs);
    if (!m_isSynchronizing)
        return false;
#ifdef _WIN32
    CHECK_TRUE( SetEvent(m_eSync) );
#endif
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


void Engine::pushInputEvent(const yamy::platform::KeyEvent &event)
{
#ifdef _WIN32
    yamy::platform::waitForObject(m_queueMutex, yamy::platform::WAIT_INFINITE);
    if (m_inputQueue) {
        m_inputQueue->push_back(event);
        SetEvent(m_readEvent);
    }
    ReleaseMutex(m_queueMutex);
#endif
}

// Convert KeyEvent to KEYBOARD_INPUT_DATA for legacy code paths
KEYBOARD_INPUT_DATA Engine::keyEventToKID(const yamy::platform::KeyEvent &event)
{
    KEYBOARD_INPUT_DATA kid;
    kid.UnitId = 0;
    kid.MakeCode = static_cast<unsigned short>(event.scanCode);
    kid.Flags = 0;
    if (!event.isKeyDown) kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
    if (event.isExtended) kid.Flags |= KEYBOARD_INPUT_DATA::E0;
    kid.Reserved = 0;
    kid.ExtraInformation = static_cast<unsigned long>(event.extraInfo);
    return kid;
}
