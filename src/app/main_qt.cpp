#include <QApplication>
#include <QMessageBox>
#include <iostream>
#include <sstream>
#include "../ui/qt/tray_icon_qt.h"
#include "../core/engine/engine.h"
#include "../core/settings/setting.h"
#include "../core/engine/msgstream.h"
#include "../platform/linux/window_system_linux.h"
#include "../platform/linux/input_injector_linux.h"
#include "../platform/linux/input_hook_linux.h"
#include "../platform/linux/input_driver_linux.h"
#include "../utils/config_store.h"

/**
 * @brief Qt GUI entry point for Linux
 *
 * Phase 6: Full engine integration
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

    // Initialize log stream
    std::ostringstream logBuffer;
    tomsgstream log(logBuffer);

    // Initialize platform interfaces
    std::cout << "Initializing Window System..." << std::endl;
    yamy::platform::IWindowSystem* windowSystem = yamy::platform::createWindowSystem();
    if (!windowSystem) {
        QMessageBox::critical(nullptr, "YAMY", "Failed to initialize Window System");
        return 1;
    }

    std::cout << "Initializing Input Injector..." << std::endl;
    yamy::platform::IInputInjector* inputInjector = yamy::platform::createInputInjector(windowSystem);
    if (!inputInjector) {
        QMessageBox::critical(nullptr, "YAMY", "Failed to initialize Input Injector");
        delete windowSystem;
        return 1;
    }

    std::cout << "Initializing Input Hook..." << std::endl;
    yamy::platform::IInputHook* inputHook = yamy::platform::createInputHook();
    if (!inputHook) {
        QMessageBox::critical(nullptr, "YAMY", "Failed to initialize Input Hook");
        delete inputInjector;
        delete windowSystem;
        return 1;
    }

    std::cout << "Initializing Input Driver..." << std::endl;
    yamy::platform::IInputDriver* inputDriver = yamy::platform::createInputDriver();
    if (!inputDriver) {
        QMessageBox::critical(nullptr, "YAMY", "Failed to initialize Input Driver");
        delete inputHook;
        delete inputInjector;
        delete windowSystem;
        return 1;
    }

    // Initialize config store
    std::cout << "Initializing Config Store..." << std::endl;
    ConfigStore* configStore = new ConfigStore();

    // Create engine instance
    std::cout << "Creating Engine..." << std::endl;
    Engine* engine = new Engine(log, windowSystem, configStore, inputInjector, inputHook, inputDriver);

    // Start engine
    std::cout << "Starting Engine..." << std::endl;
    engine->start();
    engine->enable(true); // Enable by default

    // Create and show tray icon
    TrayIconQt trayIcon(engine);
    trayIcon.show();

    // Show startup notification
    trayIcon.showNotification(
        "YAMY",
        "YAMY started successfully",
        QSystemTrayIcon::Information
    );

    std::cout << "YAMY Qt GUI initialized. Running..." << std::endl;

    // Run Qt event loop
    int result = app.exec();

    // Cleanup
    std::cout << "Shutting down YAMY..." << std::endl;
    engine->stop();
    delete engine;
    delete configStore;
    delete inputDriver;
    delete inputHook;
    delete inputInjector;
    delete windowSystem;

    std::cout << "YAMY exited successfully." << std::endl;
    return result;
}
