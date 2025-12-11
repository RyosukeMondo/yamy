#include <QApplication>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QWidget>
#include <iostream>
#include <sys/stat.h>
#include "ui/qt/tray_icon_qt.h"
#include "core/settings/session_manager.h"

// Stub Engine class for Qt GUI (until core refactoring is complete)
class Engine {
public:
    bool getIsEnabled() const { return m_enabled; }
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }
    void start() { m_running = true; }
    void stop() { m_running = false; }
    bool isRunning() const { return m_running; }
    const std::string& getConfigPath() const { return m_configPath; }
    void setConfigPath(const std::string& path) { m_configPath = path; }
private:
    bool m_enabled = true;
    bool m_running = false;
    std::string m_configPath;
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
