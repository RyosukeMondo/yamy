#include <QCoreApplication>
#include <QCommandLineParser>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QTimer>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <memory>

#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

#include "utils/crash_handler.h"
#include "engine_adapter.h"
#include "core/engine/engine.h"
#include "core/settings/session_manager.h"
#include "core/settings/config_manager.h"
#include "core/plugin_manager.h"
#include "core/platform/input_hook_interface.h"
#include "core/platform/input_injector_interface.h"
#include "core/platform/window_system_interface.h"
#include "core/platform/input_driver_interface.h"
#include "utils/msgstream.h"
#include "utils/qsettings_config_store.h"

#ifdef _WIN32
#include "platform/windows/ipc_control_server.h"
#else
#include "platform/linux/ipc_control_server.h"
#endif

struct CommandLineOptions {
    bool noRestore = false;
};

static std::ofstream* g_logStream = nullptr;

static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    if (!g_logStream) {
        return;
    }

    QByteArray localMsg = msg.toLocal8Bit();

    const char* file = context.file ? context.file : "";
    const char* function = context.function ? context.function : "";

    std::string typeStr;
    switch (type) {
    case QtDebugMsg:    typeStr = "Debug"; break;
    case QtInfoMsg:     typeStr = "Info"; break;
    case QtWarningMsg:  typeStr = "Warning"; break;
    case QtCriticalMsg: typeStr = "Critical"; break;
    case QtFatalMsg:    typeStr = "Fatal"; break;
    }

    (*g_logStream) << "[" << typeStr << "] " << localMsg.constData();
    if (context.file) {
        (*g_logStream) << " (" << file << ":" << context.line << ", " << function << ")";
    }
    (*g_logStream) << std::endl;
}

static CommandLineOptions parseCommandLine(const QCoreApplication& app) {
    CommandLineOptions options;

    QCommandLineParser parser;
    parser.setApplicationDescription("YAMY - Keyboard Remapper (headless daemon)");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption noRestoreOption(
        QStringList() << "no-restore",
        "Skip session restoration (do not restore previous config and engine state)"
    );
    parser.addOption(noRestoreOption);

    parser.process(app);

    options.noRestore = parser.isSet(noRestoreOption);

    return options;
}

static bool restoreSessionState(EngineAdapter* engine, const CommandLineOptions& options) {
    if (options.noRestore) {
        std::cout << "Session restore skipped (--no-restore flag)" << std::endl;
        return false;
    }

    yamy::SessionManager& session = yamy::SessionManager::instance();

    if (!session.hasSession()) {
        std::cout << "No previous session found" << std::endl;
        return false;
    }

    std::cout << "Restoring previous session..." << std::endl;

    if (!session.restoreSession()) {
        std::cout << "Warning: Failed to restore session (corrupt or invalid)" << std::endl;
        return false;
    }

    const yamy::SessionData& data = session.data();
    bool restored = false;

    if (!data.activeConfigPath.empty()) {
        struct stat st;
        if (stat(data.activeConfigPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            std::cout << "Loading previous config: " << data.activeConfigPath << std::endl;
            if (engine->loadConfig(data.activeConfigPath)) {
                ConfigManager& configMgr = ConfigManager::instance();
                configMgr.addConfig(data.activeConfigPath);
                configMgr.setActiveConfig(data.activeConfigPath);
                restored = true;
            } else {
                std::cout << "Warning: Failed to load previous config: "
                          << data.activeConfigPath << std::endl;
            }
        } else {
            std::cout << "Warning: Previous config not found: "
                      << data.activeConfigPath << std::endl;
        }
    }

    if (data.engineWasRunning) {
        engine->start();
        engine->enable();
        std::cout << "Restored engine state: running" << std::endl;
        restored = true;
    }

    if (restored) {
        std::cout << "Session restored successfully" << std::endl;
    }

    return restored;
}

static std::string initLogFile() {
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (logDir.isEmpty()) {
        logDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    }

    QDir dir(logDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString logPath = dir.filePath("yamy-daemon.log");
    g_logStream = new std::ofstream(logPath.toStdString(), std::ios::app);
    if (g_logStream->is_open()) {
        (*g_logStream) << "----- YAMY headless daemon start: "
                       << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString()
                       << " -----" << std::endl;
        qInstallMessageHandler(messageHandler);
    } else {
        delete g_logStream;
        g_logStream = nullptr;
        std::cerr << "Warning: Failed to open log file at " << logPath.toStdString() << std::endl;
    }

    return logPath.toStdString();
}

int main(int argc, char* argv[]) {
#ifndef _WIN32
    yamy::CrashHandler::install();
    yamy::CrashHandler::setVersion("0.04");
#endif

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("YAMY");
    QCoreApplication::setApplicationVersion("0.04");
    QCoreApplication::setOrganizationName("YAMY");

    std::string logPath = initLogFile();

    CommandLineOptions cmdOptions = parseCommandLine(app);

    std::cout << "Starting YAMY headless daemon" << std::endl;
    if (!logPath.empty()) {
        std::cout << "Log: " << logPath << std::endl;
    }

    yamy::platform::IWindowSystem* windowSystem = yamy::platform::createWindowSystem();
    yamy::platform::IInputInjector* inputInjector = yamy::platform::createInputInjector(windowSystem);
    yamy::platform::IInputHook* inputHook = yamy::platform::createInputHook();
    yamy::platform::IInputDriver* inputDriver = yamy::platform::createInputDriver();

    // Use QSettingsConfigStore for persistence
    // This allows the engine to save its configuration list
    ConfigStore* configStore = new QSettingsConfigStore("YAMY", "YAMY");

    static tomsgstream logStream(0, nullptr);

    Engine* realEngine = new Engine(
        logStream,
        windowSystem,
        configStore,
        inputInjector,
        inputHook,
        inputDriver
    );

    EngineAdapter* engine = new EngineAdapter(realEngine);

    bool sessionRestored = restoreSessionState(engine, cmdOptions);
    if (!sessionRestored) {
        std::cout << "No session restored; starting engine with defaults" << std::endl;
        engine->start();
        engine->enable();
    }

    yamy::core::PluginManager& pluginManager = yamy::core::PluginManager::instance();
    if (pluginManager.initialize(realEngine)) {
        auto loadedPlugins = pluginManager.getLoadedPlugins();
        if (loadedPlugins.empty()) {
            std::cout << "No plugins loaded (plugin directory: "
                      << yamy::core::PluginManager::getPluginDirectory() << ")" << std::endl;
        } else {
            std::cout << "Loaded " << loadedPlugins.size() << " plugin(s)" << std::endl;
        }
    } else {
        std::cerr << "Warning: Plugin system initialization failed" << std::endl;
    }

    yamy::platform::IPCControlServer controlServer;
    controlServer.setCommandCallback(
        [engine](yamy::platform::ControlCommand cmd, const std::string& data)
            -> yamy::platform::ControlResult {
        yamy::platform::ControlResult result;

        switch (cmd) {
            case yamy::platform::ControlCommand::Reload: {
                std::cout << "IPC: Received reload command";
                if (!data.empty()) {
                    std::cout << " (config: " << data << ")";
                }
                std::cout << std::endl;

                try {
                    bool loadSuccess = false;
                    std::string configPath;

                    if (!data.empty()) {
                        configPath = data;
                        loadSuccess = engine->loadConfig(data);
                    } else {
                        configPath = engine->getConfigPath();
                        if (!configPath.empty()) {
                            loadSuccess = engine->loadConfig(configPath);
                        } else {
                            result.success = false;
                            result.message = "No configuration loaded. Provide a config path to load.";
                            break;
                        }
                    }

                    if (loadSuccess) {
                        ConfigManager& configMgr = ConfigManager::instance();
                        configMgr.addConfig(configPath);
                        configMgr.setActiveConfig(configPath);

                        result.success = true;
                        result.message = "Configuration loaded successfully: " + configPath;
                    } else {
                        result.success = false;
                        result.message = "Failed to load configuration: " + configPath;
                    }
                } catch (const std::exception& e) {
                    result.success = false;
                    result.message = std::string("Error loading configuration: ") + e.what();
                } catch (...) {
                    result.success = false;
                    result.message = "Unknown error loading configuration";
                }
                break;
            }

            case yamy::platform::ControlCommand::Stop:
                std::cout << "IPC: Received stop command" << std::endl;
                engine->stop();
                result.success = true;
                result.message = "Engine stopped";
                break;

            case yamy::platform::ControlCommand::Start:
                std::cout << "IPC: Received start command" << std::endl;
                engine->start();
                result.success = true;
                result.message = "Engine started";
                break;

            case yamy::platform::ControlCommand::GetStatus: {
                std::cout << "IPC: Received status command" << std::endl;
                result.success = true;
                result.message = engine->getStatusJson();
                break;
            }

            case yamy::platform::ControlCommand::GetConfig: {
                std::cout << "IPC: Received config command" << std::endl;
                result.success = true;
                result.message = engine->getConfigJson();
                break;
            }

            case yamy::platform::ControlCommand::GetKeymaps: {
                std::cout << "IPC: Received keymaps command" << std::endl;
                result.success = true;
                result.message = engine->getKeymapsJson();
                break;
            }

            case yamy::platform::ControlCommand::GetMetrics: {
                std::cout << "IPC: Received metrics command" << std::endl;
                result.success = true;
                result.message = engine->getMetricsJson();
                break;
            }

            default:
                result.success = false;
                result.message = "Unknown command";
                break;
        }

        return result;
    });

    if (controlServer.start()) {
        std::cout << "IPC control server started at: " << controlServer.socketPath() << std::endl;
    } else {
        std::cerr << "Warning: Failed to start IPC control server" << std::endl;
    }

    // Initialize IPC channel AFTER event loop starts to avoid Qt threading issues
    // QTimer::singleShot(0, ...) schedules execution on next event loop iteration
    QTimer::singleShot(0, [realEngine]() {
        realEngine->initializeIPC();
    });

    int result = app.exec();

    controlServer.stop();

    std::cout << "Saving session state..." << std::endl;
    yamy::SessionManager& session = yamy::SessionManager::instance();
    session.setActiveConfig(engine->getConfigPath());
    session.setEngineRunning(engine->isRunning() && engine->getIsEnabled());
    if (session.saveSession()) {
        std::cout << "Session saved successfully" << std::endl;
    } else {
        std::cout << "Warning: Failed to save session" << std::endl;
    }

    std::cout << "Shutting down plugin system..." << std::endl;
    pluginManager.shutdown();

    delete engine;

    std::cout << "YAMY headless daemon exited." << std::endl;
    return result;
}
