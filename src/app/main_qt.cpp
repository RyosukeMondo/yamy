#include <QApplication>
#include <QMessageBox>
#include "../ui/qt/tray_icon_qt.h"

/**
 * @brief Qt GUI entry point for Linux
 *
 * Phase 3: Minimal implementation with system tray icon
 * Phase 6: Will add full engine integration
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

    // Create engine instance (stub for Phase 3)
    // Will be replaced with real engine in Phase 6
    Engine* engine = nullptr;

    // Create and show tray icon
    TrayIconQt trayIcon(engine);
    trayIcon.show();

    // Show startup notification
    trayIcon.showNotification(
        "YAMY",
        "YAMY started - Phase 3 (Tray Icon Test)",
        QSystemTrayIcon::Information
    );

    return app.exec();
}
