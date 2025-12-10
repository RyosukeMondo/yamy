#include <QApplication>
#include <QMessageBox>
#include <iostream>
#include "ui/qt/tray_icon_qt.h"

// Stub Engine class for Qt GUI (until core refactoring is complete)
class Engine {
public:
    bool getIsEnabled() const { return m_enabled; }
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }
private:
    bool m_enabled = true;
};

/**
 * @brief Qt GUI entry point for Linux
 *
 * Phase 7: Standalone Qt GUI build (engine integration pending core refactoring)
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

    // Create and show tray icon
    TrayIconQt trayIcon(engine);
    trayIcon.show();

    // Show startup notification
    trayIcon.showNotification(
        "YAMY",
        "YAMY Qt GUI started (demo mode)",
        QSystemTrayIcon::Information
    );

    std::cout << "YAMY Qt GUI initialized. Running..." << std::endl;

    // Run Qt event loop
    int result = app.exec();

    // Cleanup
    std::cout << "Shutting down YAMY..." << std::endl;
    delete engine;

    std::cout << "YAMY exited successfully." << std::endl;
    return result;
}
