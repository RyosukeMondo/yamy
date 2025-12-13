#include <QApplication>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <chrono>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#include "ui/qt/tray_icon_qt.h"
#include "ui/qt/crash_report_dialog.h"
#include "core/settings/session_manager.h"
#include "core/settings/config_manager.h"
#include "core/plugin_manager.h"
#ifdef _WIN32
#include "platform/windows/ipc_control_server.h"
#else
#include "platform/linux/ipc_control_server.h"
#endif
#include "utils/crash_handler.h"
#include "engine_adapter.h"
#include "core/engine/engine.h"
#include "core/platform/input_hook_interface.h"
#include "core/platform/input_injector_interface.h"
#include "core/platform/window_system_interface.h"
#include "core/platform/input_driver_interface.h"
#include "utils/msgstream.h"
#include <memory>

/// Command line options structure
struct CommandLineOptions {
    bool noRestore = false;  /// Skip session restoration if true
};

/// Parse command line arguments
/// @param app QApplication instance
/// @return Parsed command line options
static CommandLineOptions parseCommandLine(const QApplication& app) {
    CommandLineOptions options;

    QCommandLineParser parser;
    parser.setApplicationDescription("YAMY - Keyboard Remapper for Linux");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption noRestoreOption(
        QStringList() << "no-restore",
        "Skip session restoration (don't restore previous config and engine state)"
    );
    parser.addOption(noRestoreOption);

    parser.process(app);

    options.noRestore = parser.isSet(noRestoreOption);

    return options;
}

/// Restore session state from disk
/// @param engine EngineAdapter instance to configure based on restored session
/// @param options Command line options (to check --no-restore)
/// @return true if session was restored, false otherwise
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

    // Restore active configuration path
    if (!data.activeConfigPath.empty()) {
        // Check if the config file still exists
        struct stat st;
        if (stat(data.activeConfigPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            std::cout << "Loading previous config: " << data.activeConfigPath << std::endl;
            if (engine->loadConfig(data.activeConfigPath)) {
                std::cout << "Config loaded successfully" << std::endl;

                // Sync ConfigManager with the loaded config
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

    // Restore engine running state
    if (data.engineWasRunning) {
        engine->start();
        engine->enable();
        std::cout << "Restored engine state: running" << std::endl;
        restored = true;
    }

    // Log window positions restored (actual restoration happens when dialogs open)
    if (!data.windowPositions.empty()) {
        std::cout << "Restored " << data.windowPositions.size()
                  << " window position(s)" << std::endl;
    }

    if (restored) {
        std::cout << "Session restored successfully" << std::endl;
    }

    return restored;
}

/// Restore window position for a widget from session data
/// @param widget Widget to restore position for
/// @param windowName Name used to identify this window in session data
static void restoreWindowPosition(QWidget* widget, const std::string& windowName) {
    yamy::SessionManager& session = yamy::SessionManager::instance();
    yamy::WindowPosition pos = session.getWindowPosition(windowName);

    if (pos.valid) {
        widget->setGeometry(pos.x, pos.y, pos.width, pos.height);
    }
}

/// Save window position to session data
/// @param widget Widget to save position for
/// @param windowName Name used to identify this window in session data
static void saveWindowPosition(QWidget* widget, const std::string& windowName) {
    yamy::SessionManager& session = yamy::SessionManager::instance();
    QRect geom = widget->geometry();
    session.saveWindowPosition(windowName, geom.x(), geom.y(),
                               geom.width(), geom.height());
}

/**
 * @brief Qt GUI entry point for Linux
 *
 * Phase 7: Standalone Qt GUI build (engine integration pending core refactoring)
 *
 * Features:
 * - Session restore: Automatically restores last config and engine state
 * - Command line: --no-restore flag to skip session restoration
 *
 * NOTE: Full engine integration requires core YAMY refactoring to remove
 * Windows-specific dependencies (HWND, PostMessage, SW_*, etc.)
// ... (comments)
 */
// Global log stream pointer for message handler
static std::ofstream* g_logStream = nullptr;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (!g_logStream) return;

    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    
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

int main(int argc, char* argv[])
{
    // Initialize logging functionality immediately
    std::string logPath;
#ifdef _WIN32
    char path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, path, MAX_PATH) != 0) {
        std::string exePath(path);
        std::string exeDir = exePath.substr(0, exePath.find_last_of("\\/"));
        std::string localLogDir = exeDir + "\\logs";
        
        struct stat st;
        if (stat(localLogDir.c_str(), &st) == 0 && (st.st_mode & S_IFDIR)) {
             logPath = localLogDir + "\\yamy.log";
        }
    }
#endif

    // If local log exists, start logging PRE-INIT
    if (!logPath.empty()) {
        static std::ofstream debugLog(logPath, std::ios::app);
        g_logStream = &debugLog;
        qInstallMessageHandler(messageHandler);
    }
    
    // Install crash handler (Linux only)
#ifndef _WIN32
    yamy::CrashHandler::install();
    yamy::CrashHandler::setVersion("0.04");
#endif
    
    QApplication app(argc, argv);
    
    // Resume logging (or start fallback logging)
    QString debugLogPath;
    if (!logPath.empty()) {
        debugLogPath = QString::fromStdString(logPath);
        std::ofstream debugLog(logPath, std::ios::app);
        // debugLog << "MAIN: QApplication created." << std::endl; // Reduced noise
    } else {
        // Fallback to temp directory (safe now that QApplication exists)
        debugLogPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/yamy-debug.log";
        std::ofstream debugLog(debugLogPath.toStdString(), std::ios::app);
        debugLog << "MAIN: Entry point (Fallback). Log path: " << debugLogPath.toStdString() << std::endl;
        debugLog << "MAIN: QApplication created." << std::endl;
    }

    // Set application metadata
    QApplication::setApplicationName("YAMY");
    QApplication::setApplicationVersion("0.04");
    QApplication::setOrganizationName("YAMY");

    // Parse command line arguments
    CommandLineOptions cmdOptions = parseCommandLine(app);

    // Important: Don't quit when last window closes (we're a tray app)
    app.setQuitOnLastWindowClosed(false);

    // Check if system tray is available
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(
            nullptr,
            "YAMY",
            "System tray is not available on this system.\n"
            "YAMY requires a system tray to run."
        );
        return 1;
    }

    std::ofstream(debugLogPath.toStdString(), std::ios::app) << "MAIN: Starting YAMY" << std::endl;
    std::cout << "Starting YAMY (Qt GUI)" << std::endl;

    // Create platform implementations using factory functions
    std::ofstream(debugLogPath.toStdString(), std::ios::app) << "MAIN: Creating platform implementations" << std::endl;
    std::cout << "Initializing platform implementations..." << std::endl;
    yamy::platform::IWindowSystem* windowSystem = yamy::platform::createWindowSystem();
    yamy::platform::IInputInjector* inputInjector = yamy::platform::createInputInjector(windowSystem);
    yamy::platform::IInputHook* inputHook = yamy::platform::createInputHook();
    yamy::platform::IInputDriver* inputDriver = yamy::platform::createInputDriver();

    // Create log stream
    // tomsgstream is designed for Windows PostMessage, but on Linux it doesn't output anywhere
    // TODO: Create a Linux-friendly log mechanism
    static tomsgstream logStream(0, nullptr);

    // Create real Engine with platform dependencies
    Engine* realEngine = new Engine(
        logStream,
        windowSystem,
        nullptr,  // ConfigStore - not used yet
        inputInjector,
        inputHook,
        inputDriver
    );

    // Wrap real Engine in EngineAdapter for simplified Qt GUI interface
    EngineAdapter* engine = new EngineAdapter(realEngine);

    // Restore session state (unless --no-restore is specified)
    bool sessionRestored = restoreSessionState(engine, cmdOptions);

    // Create and show tray icon (uses real Engine directly)
    TrayIconQt trayIcon(realEngine);
    trayIcon.show();

    // Connect EngineAdapter notifications to TrayIconQt
    engine->setNotificationCallback(
        [&trayIcon](yamy::MessageType type, const std::string& data) {
            // Convert std::string to QString for Qt
            QString qdata = QString::fromStdString(data);
            // Forward to tray icon's message handler
            trayIcon.handleEngineMessage(type, qdata);
        }
    );

    // Initialize plugin system
    std::ofstream(debugLogPath.toStdString(), std::ios::app) << "MAIN: Initializing plugin system" << std::endl;
    std::cout << "Initializing plugin system..." << std::endl;
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

    // Check for crash reports from previous session
    std::ofstream(debugLogPath.toStdString(), std::ios::app) << "MAIN: Checking crash reports" << std::endl;
#ifndef _WIN32
    if (yamy::CrashReportDialog::shouldShowCrashDialog()) {
        std::vector<std::string> crashReports = yamy::CrashHandler::getCrashReports();
        if (!crashReports.empty()) {
            std::cout << "Previous crash detected, showing crash report dialog" << std::endl;
            yamy::CrashReportDialog crashDialog(crashReports);
            if (crashDialog.exec() == QDialog::Accepted) {
                // User chose to view the report
                if (crashDialog.selectedAction() == yamy::CrashReportDialog::Action::ViewReport) {
                    yamy::CrashReportViewerDialog viewer(crashDialog.currentReportPath());
                    viewer.exec();
                }
            }
        }
    }
#endif

    // Create IPC control server for yamy-ctl commands
    std::ofstream(debugLogPath.toStdString(), std::ios::app) << "MAIN: Creating IPC control server" << std::endl;
    yamy::platform::IPCControlServer controlServer;
    controlServer.setCommandCallback(
        [engine, &trayIcon](yamy::platform::ControlCommand cmd, const std::string& data)
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

                    // If config name provided, load it; otherwise reload current config
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
                        // Sync ConfigManager with the loaded config
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

    {
        std::ofstream debugLog(debugLogPath.toStdString(), std::ios::app);
        debugLog << "DEBUG: About to start IPC control server" << std::endl;
    }
    if (controlServer.start()) {
        std::ofstream debugLog(debugLogPath.toStdString(), std::ios::app);
        debugLog << "IPC control server started at: " << controlServer.socketPath() << std::endl;
        std::cout << "IPC control server started at: " << controlServer.socketPath() << std::endl;
    } else {
        std::ofstream debugLog(debugLogPath.toStdString(), std::ios::app);
        debugLog << "Warning: Failed to start IPC control server" << std::endl;
        std::cerr << "Warning: Failed to start IPC control server" << std::endl;
    }

    // Show startup notification
    QString notificationMsg = sessionRestored
        ? "YAMY started (session restored)"
        : "YAMY Qt GUI started (demo mode)";
    trayIcon.showNotification(
        "YAMY",
        notificationMsg,
        QSystemTrayIcon::Information
    );

    // Force icon refresh after a short delay to ensure tray is ready
    QTimer::singleShot(500, &trayIcon, &TrayIconQt::forceIconRefresh);

    std::cout << "YAMY Qt GUI initialized. Running..." << std::endl;

    // Run Qt event loop
    int result = app.exec();

    // Stop IPC control server
    controlServer.stop();

    // Save session state before exit
    std::cout << "Saving session state..." << std::endl;
    yamy::SessionManager& session = yamy::SessionManager::instance();
    session.setActiveConfig(engine->getConfigPath());
    session.setEngineRunning(engine->isRunning() && engine->getIsEnabled());
    if (session.saveSession()) {
        std::cout << "Session saved successfully" << std::endl;
    } else {
        std::cout << "Warning: Failed to save session" << std::endl;
    }

    // Shutdown plugin system
    std::cout << "Shutting down plugin system..." << std::endl;
    pluginManager.shutdown();

    // Cleanup
    std::cout << "Shutting down YAMY..." << std::endl;
    delete engine;  // Deletes EngineAdapter, which deletes the real Engine
    // Note: Platform implementations are owned and deleted by the real Engine

    std::cout << "YAMY exited successfully." << std::endl;
    return result;
}
