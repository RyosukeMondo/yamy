#include <QApplication>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <iostream>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <chrono>
#include "ui/qt/tray_icon_qt.h"
#include "core/settings/session_manager.h"
#include "platform/linux/ipc_control_server.h"
#include "utils/crash_handler.h"

// Stub Engine class for Qt GUI (until core refactoring is complete)
class Engine {
public:
    Engine() : m_startTime(std::chrono::steady_clock::now()) {}

    bool getIsEnabled() const { return m_enabled; }
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }
    void start() {
        m_running = true;
        m_engineStartTime = std::chrono::steady_clock::now();
    }
    void stop() { m_running = false; }
    bool isRunning() const { return m_running; }
    const std::string& getConfigPath() const { return m_configPath; }
    void setConfigPath(const std::string& path) {
        m_configPath = path;
        yamy::CrashHandler::setConfigPath(path);
    }
    uint64_t keyCount() const { return m_keyCount; }
    void incrementKeyCount() { ++m_keyCount; }

    /// Get uptime in seconds
    int64_t uptimeSeconds() const {
        if (!m_running) return 0;
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - m_engineStartTime).count();
    }

    /// Format uptime as human-readable string
    std::string uptimeString() const {
        int64_t secs = uptimeSeconds();
        int hours = static_cast<int>(secs / 3600);
        int mins = static_cast<int>((secs % 3600) / 60);
        std::ostringstream oss;
        if (hours > 0) {
            oss << hours << "h " << mins << "m";
        } else {
            oss << mins << "m";
        }
        return oss.str();
    }

    /// Get current keymap name
    std::string currentKeymap() const { return m_currentKeymap; }

    /// Set current keymap name
    void setCurrentKeymap(const std::string& name) { m_currentKeymap = name; }

    /// Get status as JSON string
    /// Returns: {"state": "running/stopped", "uptime": seconds, "config": "name", "key_count": N, "current_keymap": "name"}
    std::string getStatusJson() const {
        QJsonObject obj;
        obj["state"] = m_running ? "running" : "stopped";
        obj["uptime"] = static_cast<qint64>(uptimeSeconds());
        obj["config"] = QString::fromStdString(m_configPath);
        obj["key_count"] = static_cast<qint64>(m_keyCount);
        obj["current_keymap"] = QString::fromStdString(m_currentKeymap.empty() ? "default" : m_currentKeymap);
        return QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString();
    }

    /// Get config as JSON string
    /// Returns: {"config_path": "path", "config_name": "name", "loaded_time": "ISO8601"}
    std::string getConfigJson() const {
        QJsonObject obj;
        obj["config_path"] = QString::fromStdString(m_configPath);

        // Extract config name from path
        std::string configName = m_configPath;
        size_t lastSlash = m_configPath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            configName = m_configPath.substr(lastSlash + 1);
        }
        obj["config_name"] = QString::fromStdString(configName);
        obj["loaded_time"] = m_configLoadedTime.toString(Qt::ISODate);
        return QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString();
    }

    /// Set config loaded time (called when config is loaded/reloaded)
    void setConfigLoadedTime() {
        m_configLoadedTime = QDateTime::currentDateTime();
    }

    /// Get keymaps as JSON string
    /// Returns: {"keymaps": [{"name": "name", "window_class": "regex", "window_title": "regex"}, ...]}
    std::string getKeymapsJson() const {
        QJsonArray keymapsArray;
        for (const auto& km : m_keymaps) {
            QJsonObject kmObj;
            kmObj["name"] = QString::fromStdString(km.name);
            kmObj["window_class"] = QString::fromStdString(km.windowClass);
            kmObj["window_title"] = QString::fromStdString(km.windowTitle);
            keymapsArray.append(kmObj);
        }
        QJsonObject obj;
        obj["keymaps"] = keymapsArray;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString();
    }

    /// Keymap info structure
    struct KeymapInfo {
        std::string name;
        std::string windowClass;
        std::string windowTitle;
    };

    /// Add a keymap (stub - would be populated from real config)
    void addKeymap(const std::string& name, const std::string& windowClass = "",
                   const std::string& windowTitle = "") {
        m_keymaps.push_back({name, windowClass, windowTitle});
    }

    /// Get metrics as JSON string
    /// Returns: {"latency_avg_ns": N, "latency_p99_ns": N, "cpu_usage_percent": N}
    std::string getMetricsJson() const {
        QJsonObject obj;
        obj["latency_avg_ns"] = static_cast<qint64>(m_latencyAvgNs);
        obj["latency_p99_ns"] = static_cast<qint64>(m_latencyP99Ns);
        obj["latency_max_ns"] = static_cast<qint64>(m_latencyMaxNs);
        obj["cpu_usage_percent"] = m_cpuUsagePercent;
        obj["keys_per_second"] = m_keysPerSecond;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact).toStdString();
    }

    /// Update metrics (stub - would be populated from real metrics)
    void updateMetrics(uint64_t avgNs, uint64_t p99Ns, uint64_t maxNs,
                       double cpuPercent, double keysPerSec) {
        m_latencyAvgNs = avgNs;
        m_latencyP99Ns = p99Ns;
        m_latencyMaxNs = maxNs;
        m_cpuUsagePercent = cpuPercent;
        m_keysPerSecond = keysPerSec;
    }

private:
    bool m_enabled = true;
    bool m_running = false;
    std::string m_configPath;
    std::string m_currentKeymap;
    uint64_t m_keyCount = 0;
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_engineStartTime;
    QDateTime m_configLoadedTime;
    std::vector<KeymapInfo> m_keymaps;

    // Metrics (stub values)
    uint64_t m_latencyAvgNs = 0;
    uint64_t m_latencyP99Ns = 0;
    uint64_t m_latencyMaxNs = 0;
    double m_cpuUsagePercent = 0.0;
    double m_keysPerSecond = 0.0;
};

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
/// @param engine Engine instance to configure based on restored session
/// @param options Command line options (to check --no-restore)
/// @return true if session was restored, false otherwise
static bool restoreSessionState(Engine* engine, const CommandLineOptions& options) {
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
            engine->setConfigPath(data.activeConfigPath);
            std::cout << "Restored config: " << data.activeConfigPath << std::endl;
            restored = true;
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
 */
int main(int argc, char* argv[])
{
    // Install crash handler as early as possible
    yamy::CrashHandler::install();
    yamy::CrashHandler::setVersion("0.04");

    QApplication app(argc, argv);

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

    std::cout << "Starting YAMY on Linux (Qt GUI)" << std::endl;
    std::cout << "Note: Using stub engine (full integration pending core refactoring)" << std::endl;

    // Create stub engine instance
    Engine* engine = new Engine();

    // Restore session state (unless --no-restore is specified)
    bool sessionRestored = restoreSessionState(engine, cmdOptions);

    // Create and show tray icon
    TrayIconQt trayIcon(engine);
    trayIcon.show();

    // Create IPC control server for yamy-ctl commands
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

                // If config name provided, set it
                if (!data.empty()) {
                    engine->setConfigPath(data);
                }

                // For stub engine, just report success
                result.success = true;
                result.message = "Configuration reloaded";
                if (!engine->getConfigPath().empty()) {
                    result.message += ": " + engine->getConfigPath();
                }
                break;
            }

            case yamy::platform::ControlCommand::Stop:
                std::cout << "IPC: Received stop command" << std::endl;
                if (engine->isRunning()) {
                    engine->stop();
                    engine->disable();
                    result.success = true;
                    result.message = "Engine stopped";
                } else {
                    result.success = true;
                    result.message = "Engine was already stopped";
                }
                break;

            case yamy::platform::ControlCommand::Start:
                std::cout << "IPC: Received start command" << std::endl;
                if (!engine->isRunning()) {
                    engine->enable();
                    engine->start();
                    result.success = true;
                    result.message = "Engine started";
                } else {
                    result.success = true;
                    result.message = "Engine was already running";
                }
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

    // Start IPC control server
    if (controlServer.start()) {
        std::cout << "IPC control server started at: " << controlServer.socketPath() << std::endl;
    } else {
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

    // Cleanup
    std::cout << "Shutting down YAMY..." << std::endl;
    delete engine;

    std::cout << "YAMY exited successfully." << std::endl;
    return result;
}
