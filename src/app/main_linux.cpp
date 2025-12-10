#include <iostream>
#include <vector>
#include <string>

// Include stub interfaces to verify linking
#include "core/platform/window_system_interface.h"
#include "core/platform/input_injector_interface.h"
#include "core/platform/input_hook_interface.h"
#include "core/platform/input_driver_interface.h"
#include "core/platform/thread.h"

int main(int argc, char* argv[]) {
    std::cout << "Starting YAMY on Linux (Stub Backend)" << std::endl;

    // Initialize Platform Interfaces
    std::cout << "Initializing Window System..." << std::endl;
    yamy::platform::IWindowSystem* windowSystem = yamy::platform::createWindowSystem();

    std::cout << "Initializing Input Injector..." << std::endl;
    yamy::platform::IInputInjector* inputInjector = yamy::platform::createInputInjector(windowSystem);

    std::cout << "Initializing Input Hook..." << std::endl;
    yamy::platform::IInputHook* inputHook = yamy::platform::createInputHook();

    std::cout << "Initializing Input Driver..." << std::endl;
    yamy::platform::IInputDriver* inputDriver = yamy::platform::createInputDriver();

    // Basic connectivity test
    if (windowSystem) {
        windowSystem->getForegroundWindow(); // Should print STUB
    }

    if (inputInjector) {
        inputInjector->mouseMove(10, 10); // Should print STUB
    }

    // Clean up
    delete inputDriver;
    delete inputHook;
    delete inputInjector;
    delete windowSystem;

    std::cout << "YAMY Linux Stub exited successfully." << std::endl;
    return 0;
}
