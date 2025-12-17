//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_lifecycle.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "../platform/ipc_channel_factory.h"
#include "hook.h"
#include "mayurc.h"
#include "../platform/message_constants.h"
#include "../platform/sync.h"
#include "../platform/thread.h"
#include "core/logging/logger.h"
#include "../../utils/metrics.h"
#ifdef _WIN32
#include "../../utils/debug_console.h"
#endif

#include <iomanip>
#include <string>
#include <chrono>
#include <thread>
#include "../platform/ipc_defs.h"
#include "../notification_dispatcher.h"
#include <gsl/gsl>

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
        m_currentConfigPath(),
        m_windowSystem(i_windowSystem),
        m_configStore(i_configStore),
        m_configSwitchCallback(nullptr),
        m_inputInjector(i_inputInjector),
        m_inputHook(i_inputHook),
        m_inputDriver(i_inputDriver),
        m_inputQueue(nullptr),
        m_queueMutex(nullptr),
        m_readEvent(nullptr),
        m_ol(nullptr),
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
    // Preconditions
    Expects(i_windowSystem != nullptr);
    // Note: i_configStore can be nullptr - only needed for config switching (Engine::switchConfiguration)
    Expects(i_inputInjector != nullptr);
    Expects(i_inputHook != nullptr);
    Expects(i_inputDriver != nullptr);

    m_state = yamy::EngineState::Stopped;
    // Enable receiving WM_COPYDATA from lower integrity processes
    m_windowSystem->changeMessageFilter(yamy::platform::MSG_COPYDATA,
                                       yamy::platform::MSGFLT_ADD);
    
    // IPC channel creation - but DON'T call listen() yet
    // listen() must be called AFTER Qt event loop starts to avoid QSocketNotifier threading issues
    m_ipcChannel = yamy::platform::createIPCChannel("yamy-engine");
    if (m_ipcChannel) {
#if defined(QT_CORE_LIB)
        // Connect IPC signal to handler
        QObject::connect(m_ipcChannel.get(), &yamy::platform::IIPCChannel::messageReceived,
                         [this](const yamy::ipc::Message& msg) {
                             this->handleIpcMessage(msg);
                         });
#endif
        // NOTE: listen() will be called later via initializeIPC() after event loop starts
    }

    for (size_t i = 0; i < NUMBER_OF(m_lastPressedKey); ++ i)
        m_lastPressedKey[i] = nullptr;

    // set default lock state
    for (int i = 0; i < Modifier::Type_end; ++ i)
        m_currentLock.dontcare(static_cast<Modifier::Type>(i));
    for (int i = Modifier::Type_Lock0; i <= Modifier::Type_Lock9; ++ i)
        m_currentLock.release(static_cast<Modifier::Type>(i));

    // create event for sync
    CHECK_TRUE( m_eSync = yamy::platform::createEvent(false, false) );

#ifdef _WIN32
    // create named pipe for &SetImeString (Windows-only feature)
    m_hookPipe = CreateNamedPipe(addSessionId(HOOK_PIPE_NAME).c_str(),
                                 PIPE_ACCESS_OUTBOUND,
                                 PIPE_TYPE_BYTE, 1,
                                 0, 0, 0, nullptr);
#endif
    StrExprArg::setSystem(this);
}


Engine::~Engine() {
    CHECK_TRUE( yamy::platform::destroyEvent(m_eSync) );

#ifdef _WIN32
    // Clean up OVERLAPPED structure
    if (m_ol) {
        delete reinterpret_cast<OVERLAPPED*>(m_ol);
        m_ol = nullptr;
    }

    // destroy named pipe for &SetImeString (Windows-only feature)
    if (m_hookPipe && m_hookPipe != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(m_hookPipe);
        CHECK_TRUE( CloseHandle(m_hookPipe) );
    }
#endif
}


// start keyboard handler thread
void Engine::start() {
#ifdef _WIN32
    yamy::debug::DebugConsole::LogInfo("Engine::start() called");
#endif
    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine", "Engine::start() called");
    setState(yamy::EngineState::Loading);
    notifyGUI(yamy::MessageType::EngineStarting);

    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine", "Starting engine...");
#ifdef _WIN32
    yamy::debug::DebugConsole::LogInfo("Engine: Starting performance metrics...");
#endif
    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine", "Starting performance metrics...");
    // Start performance metrics collection with 60-second reporting interval
    yamy::metrics::PerformanceMetrics::instance().startPeriodicLogging(60);

#ifdef _WIN32
    yamy::debug::DebugConsole::LogInfo("Engine: Installing input hook...");
#endif
    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine", "Installing input hook...");
    std::cerr << "[DEBUG] Engine: About to call m_inputHook->install(), m_inputHook=" << m_inputHook << std::endl;
    if (!m_inputHook) {
        std::cerr << "[DEBUG] Engine: ERROR - m_inputHook is NULL!" << std::endl;
    }
    m_inputHook->install(
        [this](const yamy::platform::KeyEvent& event) {
            // Pass KeyEvent directly to the queue
            this->pushInputEvent(event);
            // Only block events if we have a configuration loaded
            // Otherwise pass through to allow normal keyboard operation
            return (this->m_setting != nullptr);
        },
        [this](const yamy::platform::MouseEvent& e) {
            // Mouse event handler (currently unused)
            // Pass through - we don't remap mouse events
            return false;
        }
    );

#ifdef _WIN32
    yamy::debug::DebugConsole::LogInfo("Engine: Creating input queue and synchronization objects...");
    yamy::debug::DebugConsole::LogInfo("Engine: Creating input queue (deque)...");
#endif
    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine", "Creating input queue and synchronization objects...");
    CHECK_TRUE( m_inputQueue = new std::deque<yamy::platform::KeyEvent> );
#ifdef _WIN32
    yamy::debug::DebugConsole::LogInfo("Engine: Creating mutex...");
#endif
    CHECK_TRUE( m_queueMutex = yamy::platform::createMutex() );
#ifdef _WIN32
    yamy::debug::DebugConsole::LogInfo("Engine: Creating event...");
#endif
    CHECK_TRUE( m_readEvent = yamy::platform::createEvent(true, false) );
#ifdef _WIN32
    yamy::debug::DebugConsole::LogInfo("Engine: Synchronization objects created successfully!");
    yamy::debug::DebugConsole::LogInfo("Engine: Allocating OVERLAPPED structure...");

    // Allocate OVERLAPPED structure for async I/O
    m_ol = new OVERLAPPED();
    if (m_ol) {
        OVERLAPPED* pOl = reinterpret_cast<OVERLAPPED*>(m_ol);
        ZeroMemory(pOl, sizeof(OVERLAPPED));
        pOl->hEvent = m_readEvent;
        yamy::debug::DebugConsole::LogInfo("Engine: OVERLAPPED structure allocated and configured!");
    } else {
        yamy::debug::DebugConsole::LogError("Engine: Failed to allocate OVERLAPPED structure!");
    }

    yamy::debug::DebugConsole::LogInfo("Engine: Opening input driver...");
#endif
    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine", "Opening input driver...");
    m_inputDriver->open(m_readEvent);
#ifdef _WIN32
    yamy::debug::DebugConsole::LogInfo("Engine: Input driver opened successfully!");
#endif

#ifdef _WIN32
    yamy::debug::DebugConsole::LogInfo("Engine: Creating keyboard handler thread...");
#endif
    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine", "Creating keyboard handler thread...");
    CHECK_TRUE( m_threadHandle = yamy::platform::createThread(keyboardHandler, this) );
#ifdef _WIN32
    yamy::debug::DebugConsole::LogInfo("Engine: Keyboard handler thread created!");
    yamy::debug::DebugConsole::LogInfo("Engine: Creating performance metrics thread...");
#endif
    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine", "Creating performance metrics thread...");
    m_isPerfThreadRunning = true;
    CHECK_TRUE( m_perfThreadHandle = yamy::platform::createThread(perfMetricsHandler, this) );
#ifdef _WIN32
    yamy::debug::DebugConsole::LogInfo("Engine: Performance metrics thread created!");
#endif

#ifdef _WIN32
    yamy::debug::DebugConsole::LogInfo("Engine: Engine started successfully!");
#endif
    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine", "Engine started successfully!");
    setState(yamy::EngineState::Running);
    notifyGUI(yamy::MessageType::EngineStarted);
}


// Initialize IPC channel - must be called AFTER Qt event loop starts
void Engine::initializeIPC() {
    if (m_ipcChannel) {
        m_ipcChannel->listen();
        yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine", "IPC channel initialized and listening");
    }

    // Set up LockState notification callback to send IPC messages
    m_modifierState.setNotificationCallback([this](const uint32_t lockBits[8]) {
        // Create LockStatusMessage and send via IPC
        yamy::ipc::LockStatusMessage msg;
        std::memcpy(msg.lockBits, lockBits, sizeof(msg.lockBits));
        this->notifyGUI(yamy::MessageType::LockStatusUpdate, &msg, sizeof(msg));
    });
}


// stop keyboard handler thread
void Engine::stop() {
    notifyGUI(yamy::MessageType::EngineStopping);

    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Engine", "Stopping engine...");
    // Stop performance metrics collection
    yamy::metrics::PerformanceMetrics::instance().stopPeriodicLogging();

    m_isPerfThreadRunning = false;
    if (m_perfThreadHandle) {
        yamy::platform::waitForObject(m_perfThreadHandle, 2000);
        CHECK_TRUE( yamy::platform::destroyThread(m_perfThreadHandle) );
        m_perfThreadHandle = nullptr;
    }

    m_inputHook->uninstall();
    m_inputDriver->close();

    yamy::platform::acquireMutex(m_queueMutex, yamy::platform::WAIT_INFINITE);
    delete m_inputQueue;
    m_inputQueue = nullptr;
    yamy::platform::setEvent(m_readEvent);
    yamy::platform::releaseMutex(m_queueMutex);

    yamy::platform::waitForObject(m_threadHandle, 2000);
    CHECK_TRUE( yamy::platform::destroyThread(m_threadHandle) );
    m_threadHandle = nullptr;

    CHECK_TRUE( yamy::platform::destroyEvent(m_readEvent) );
    m_readEvent = nullptr;

#ifdef _WIN32
    // Windows: Send null messages to attached threads to wake them on shutdown
    // Linux: Not needed - threads are properly joined or detached
    for (ThreadIds::iterator i = m_attachedThreadIds.begin();
         i != m_attachedThreadIds.end(); i++) {
         PostThreadMessage(*i, WM_NULL, 0, 0);
    }
#endif

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
    CHECK_TRUE( yamy::platform::setEvent(m_eSync) );
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
    Expects(event.scanCode <= 0xFFFF);

    yamy::platform::acquireMutex(m_queueMutex, yamy::platform::WAIT_INFINITE);
    if (m_inputQueue) {
        m_inputQueue->push_back(event);
        yamy::platform::setEvent(m_readEvent);
    }
    yamy::platform::releaseMutex(m_queueMutex);
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

    m_state = i_newState;
}

void Engine::notifyGUI(yamy::MessageType i_type, const std::string &i_data)
{
    // Dispatch to registered callbacks (plugin/extension support)
    yamy::core::NotificationDispatcher::instance().dispatch(i_type, i_data);

    if (!m_windowSystem || !m_hwndAssocWindow)
        return;

    yamy::platform::CopyData cd;
    cd.id = static_cast<uint32_t>(i_type);
    cd.size = static_cast<uint32_t>(i_data.size());
    cd.data = i_data.c_str();

    m_windowSystem->sendCopyData(nullptr, m_hwndAssocWindow, cd, yamy::platform::SendMessageFlags::NORMAL, 100, nullptr);
}

void Engine::notifyGUI(yamy::MessageType i_type, const void* i_data, size_t i_size)
{
    // Send binary data via IPC channel
    if (m_ipcChannel && m_ipcChannel->isConnected()) {
        yamy::ipc::Message msg;
        msg.type = static_cast<yamy::ipc::MessageType>(static_cast<uint32_t>(i_type));
        msg.data = i_data;
        msg.size = i_size;
        m_ipcChannel->send(msg);
    }
}

void* Engine::perfMetricsHandler(void *i_this)
{
    reinterpret_cast<Engine *>(i_this)->perfMetricsHandler();
    return nullptr;
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
