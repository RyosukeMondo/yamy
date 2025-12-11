//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_lifecycle.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "../platform/ipc_channel_factory.h"
#include "hook.h"
#include "mayurc.h"
#ifdef _WIN32
#include "windowstool.h"
#endif
#include "../platform/message_constants.h"
#include "../platform/sync.h"
#include "core/logging/logger.h"
#include "../../utils/metrics.h"

#include <iomanip>
#ifdef _WIN32
#include <process.h>
#endif
#include <string>
#include <chrono>
#include <thread>
#include <thread>
#include "../platform/ipc_defs.h"

#if defined(QT_CORE_LIB)
void Engine::playSound(yamy::audio::NotificationType type)
{
    if (m_soundManager) {
        m_soundManager->playSound(type);
    }
}
#endif

Engine::Engine(tomsgstream &i_log, yamy::platform::IWindowSystem *i_windowSystem, ConfigStore *i_configStore, yamy::platform::IInputInjector *i_inputInjector, yamy::platform::IInputHook *i_inputHook, yamy::platform::IInputDriver *i_inputDriver)
        : m_hwndAssocWindow(nullptr),
#if defined(QT_CORE_LIB)
        m_soundManager(std::make_unique<yamy::audio::SoundManager>()),
#endif
        m_setting(nullptr),
        m_windowSystem(i_windowSystem),
        m_configStore(i_configStore),
        m_configSwitchCallback(nullptr),
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
        m_isInvestigateMode(false),
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
        m_log(i_log),
        m_perfThreadHandle(nullptr),
        m_isPerfThreadRunning(false) {
    m_state = yamy::EngineState::Stopped;
    // Enable receiving WM_COPYDATA from lower integrity processes
    m_windowSystem->changeMessageFilter(yamy::platform::MSG_COPYDATA,
                                       yamy::platform::MSGFLT_ADD);
    
    m_ipcChannel = yamy::platform::createIPCChannel("yamy-engine");
    if (m_ipcChannel) {
        // In a real implementation, we would connect to a signal from the IPC channel.
        // For now, we will poll for messages in the main loop.
        m_ipcChannel->listen();
    }

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
    setState(yamy::EngineState::Loading);
    notifyGUI(yamy::MessageType::EngineStarting);

    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine", "Starting engine...");
    // Start performance metrics collection with 60-second reporting interval
    yamy::metrics::PerformanceMetrics::instance().startPeriodicLogging(60);

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
    m_isPerfThreadRunning = true;
    CHECK_TRUE( m_perfThreadHandle = reinterpret_cast<yamy::platform::ThreadHandle>(_beginthreadex(nullptr, 0, perfMetricsHandler, this, 0, nullptr)) );
#endif

    setState(yamy::EngineState::Running);
    notifyGUI(yamy::MessageType::EngineStarted);
}


// stop keyboard handler thread
void Engine::stop() {
    notifyGUI(yamy::MessageType::EngineStopping);

    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine", "Stopping engine...");
    // Stop performance metrics collection
    yamy::metrics::PerformanceMetrics::instance().stopPeriodicLogging();

    m_isPerfThreadRunning = false;
#ifdef _WIN32
    if (m_perfThreadHandle) {
        yamy::platform::waitForObject(m_perfThreadHandle, 2000);
        CHECK_TRUE( CloseHandle(m_perfThreadHandle) );
        m_perfThreadHandle = nullptr;
    }
#endif

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

    setState(yamy::EngineState::Stopped);
    notifyGUI(yamy::MessageType::EngineStopped);
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
    if (m_currentKeymap != i_keymap) {
        if (i_keymap) {
            notifyGUI(yamy::MessageType::KeymapSwitched, i_keymap->getName());
        } else {
            notifyGUI(yamy::MessageType::KeymapSwitched, "Default");
        }
    }

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

void Engine::setState(yamy::EngineState i_newState)
{
    if (m_state == i_newState)
        return;

    // TODO: Add logging for state transitions
    m_state = i_newState;
}

void Engine::notifyGUI(yamy::MessageType i_type, const std::string &i_data)
{
    if (!m_windowSystem || !m_hwndAssocWindow)
        return;

    yamy::platform::CopyData cd;
    cd.id = static_cast<uint32_t>(i_type);
    cd.size = static_cast<uint32_t>(i_data.size());
    cd.data = i_data.c_str();

    m_windowSystem->sendCopyData(nullptr, m_hwndAssocWindow, cd, yamy::platform::SendMessageFlags::NORMAL, 100, nullptr);
}

unsigned int WINAPI Engine::perfMetricsHandler(void *i_this)
{
    reinterpret_cast<Engine *>(i_this)->perfMetricsHandler();
    return 0;
}

void Engine::perfMetricsHandler()
{
    while (m_isPerfThreadRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(60));
        if (!m_isPerfThreadRunning) break;

        // Send latency report
        // This is a placeholder for actual latency reporting
        notifyGUI(yamy::MessageType::LatencyReport, "P95: 1.2ms");

        // Send CPU usage report
        // This is a placeholder for actual CPU usage reporting
        notifyGUI(yamy::MessageType::CpuUsageReport, "CPU: 5%");
    }
}
