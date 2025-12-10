//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// leak_test.cpp - Memory leak detection tests for Linux platform
//
// This test file exercises the full lifecycle of platform components to
// verify there are no memory leaks. Run with AddressSanitizer enabled:
//
//   cmake -B build -DENABLE_ASAN=ON
//   cmake --build build
//   ./build/bin/yamy_leak_test
//
// ASAN will report any memory leaks at exit.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>

#include "core/platform/window_system_interface.h"
#include "core/platform/input_injector_interface.h"
#include "core/platform/input_hook_interface.h"
#include "core/platform/input_driver_interface.h"

namespace yamy::test {

//=============================================================================
// LeakTest - Memory leak detection test fixture
//=============================================================================

class LeakTest : public ::testing::Test {
protected:
    void SetUp() override {
        // No setup needed - each test manages its own resources
    }

    void TearDown() override {
        // No teardown needed - ASAN will report leaks at exit
    }
};

//=============================================================================
// Test 1: WindowSystem lifecycle - create and destroy
//=============================================================================

TEST_F(LeakTest, WindowSystemLifecycle) {
    // Create window system
    platform::IWindowSystem* windowSystem = platform::createWindowSystem();
    ASSERT_NE(windowSystem, nullptr);

    // Exercise basic operations
    windowSystem->getForegroundWindow();

    // Cleanup
    delete windowSystem;
    // ASAN will detect if any X11 resources were leaked
}

//=============================================================================
// Test 2: WindowSystem repeated create/destroy cycles
//=============================================================================

TEST_F(LeakTest, WindowSystemRepeatedCycles) {
    for (int i = 0; i < 10; ++i) {
        platform::IWindowSystem* windowSystem = platform::createWindowSystem();
        ASSERT_NE(windowSystem, nullptr);

        // Exercise operations
        windowSystem->getForegroundWindow();

        delete windowSystem;
    }
    // Multiple cycles should not accumulate leaks
}

//=============================================================================
// Test 3: InputInjector lifecycle
//=============================================================================

TEST_F(LeakTest, InputInjectorLifecycle) {
    platform::IWindowSystem* windowSystem = platform::createWindowSystem();
    ASSERT_NE(windowSystem, nullptr);

    platform::IInputInjector* inputInjector = platform::createInputInjector(windowSystem);
    ASSERT_NE(inputInjector, nullptr);

    // Exercise basic operations (won't actually inject without proper setup)
    inputInjector->mouseMove(0, 0);

    // Cleanup in correct order
    delete inputInjector;
    delete windowSystem;
}

//=============================================================================
// Test 4: InputHook lifecycle
//=============================================================================

TEST_F(LeakTest, InputHookLifecycle) {
    platform::IInputHook* inputHook = platform::createInputHook();
    ASSERT_NE(inputHook, nullptr);

    // Install and uninstall hook - may throw if no keyboard devices are found
    // This is expected in test environments without input devices
    try {
        bool installed = inputHook->install(
            [](const platform::KeyEvent&) { return false; },
            [](const platform::MouseEvent&) { return false; }
        );

        if (installed) {
            inputHook->uninstall();
        }
    } catch (const std::exception& e) {
        // Expected in environments without keyboard devices
        std::cerr << "[INFO] InputHook install expected failure: " << e.what() << std::endl;
    }

    // Cleanup
    delete inputHook;
}

//=============================================================================
// Test 5: InputHook repeated install/uninstall cycles
//=============================================================================

TEST_F(LeakTest, InputHookRepeatedCycles) {
    platform::IInputHook* inputHook = platform::createInputHook();
    ASSERT_NE(inputHook, nullptr);

    for (int i = 0; i < 5; ++i) {
        try {
            bool installed = inputHook->install(
                [](const platform::KeyEvent&) { return false; },
                [](const platform::MouseEvent&) { return false; }
            );

            if (installed) {
                inputHook->uninstall();
            }
        } catch (const std::exception& e) {
            // Expected in environments without keyboard devices
            std::cerr << "[INFO] InputHook install expected failure (cycle " << i << "): "
                      << e.what() << std::endl;
            break; // No point continuing cycles if hardware isn't available
        }
    }

    delete inputHook;
}

//=============================================================================
// Test 6: InputDriver lifecycle
//=============================================================================

TEST_F(LeakTest, InputDriverLifecycle) {
    platform::IInputDriver* inputDriver = platform::createInputDriver();
    ASSERT_NE(inputDriver, nullptr);

    // Cleanup
    delete inputDriver;
}

//=============================================================================
// Test 7: Full platform stack lifecycle
//=============================================================================

TEST_F(LeakTest, FullPlatformStackLifecycle) {
    // Create all platform components
    platform::IWindowSystem* windowSystem = platform::createWindowSystem();
    ASSERT_NE(windowSystem, nullptr);

    platform::IInputInjector* inputInjector = platform::createInputInjector(windowSystem);
    ASSERT_NE(inputInjector, nullptr);

    platform::IInputHook* inputHook = platform::createInputHook();
    ASSERT_NE(inputHook, nullptr);

    platform::IInputDriver* inputDriver = platform::createInputDriver();
    ASSERT_NE(inputDriver, nullptr);

    // Exercise basic operations
    windowSystem->getForegroundWindow();
    inputInjector->mouseMove(0, 0);

    try {
        bool installed = inputHook->install(
            [](const platform::KeyEvent&) { return false; },
            [](const platform::MouseEvent&) { return false; }
        );
        if (installed) {
            inputHook->uninstall();
        }
    } catch (const std::exception& e) {
        // Expected in environments without keyboard devices
        std::cerr << "[INFO] InputHook install expected failure: " << e.what() << std::endl;
    }

    // Cleanup in reverse order of creation
    delete inputDriver;
    delete inputHook;
    delete inputInjector;
    delete windowSystem;
}

//=============================================================================
// Test 8: Full platform stack - repeated cycles
//=============================================================================

TEST_F(LeakTest, FullPlatformStackRepeatedCycles) {
    for (int cycle = 0; cycle < 3; ++cycle) {
        platform::IWindowSystem* windowSystem = platform::createWindowSystem();
        ASSERT_NE(windowSystem, nullptr);

        platform::IInputInjector* inputInjector = platform::createInputInjector(windowSystem);
        ASSERT_NE(inputInjector, nullptr);

        platform::IInputHook* inputHook = platform::createInputHook();
        ASSERT_NE(inputHook, nullptr);

        platform::IInputDriver* inputDriver = platform::createInputDriver();
        ASSERT_NE(inputDriver, nullptr);

        // Exercise operations
        windowSystem->getForegroundWindow();
        inputInjector->mouseMove(0, 0);

        try {
            bool installed = inputHook->install(
                [](const platform::KeyEvent&) { return false; },
                [](const platform::MouseEvent&) { return false; }
            );
            if (installed) {
                inputHook->uninstall();
            }
        } catch (const std::exception& e) {
            // Expected in environments without keyboard devices
            std::cerr << "[INFO] InputHook install expected failure (cycle " << cycle << "): "
                      << e.what() << std::endl;
        }

        // Cleanup
        delete inputDriver;
        delete inputHook;
        delete inputInjector;
        delete windowSystem;
    }
}

//=============================================================================
// Test 9: WindowSystem window operations
//=============================================================================

TEST_F(LeakTest, WindowSystemWindowOperations) {
    platform::IWindowSystem* windowSystem = platform::createWindowSystem();
    ASSERT_NE(windowSystem, nullptr);

    // Exercise various window operations that may allocate internal resources
    platform::WindowHandle hwnd = windowSystem->getForegroundWindow();

    platform::Rect rect;
    windowSystem->getWindowRect(hwnd, &rect);

    std::string text = windowSystem->getWindowText(hwnd);
    std::string title = windowSystem->getTitleName(hwnd);
    std::string className = windowSystem->getClassName(hwnd);

    windowSystem->getWindowThreadId(hwnd);
    windowSystem->getWindowProcessId(hwnd);

    // getMonitorCount/getMonitorRect may allocate internal resources
    int monitorCount = windowSystem->getMonitorCount();
    for (int i = 0; i < monitorCount; ++i) {
        platform::Rect monitorRect;
        windowSystem->getMonitorRect(i, &monitorRect);
        windowSystem->getMonitorWorkArea(i, &monitorRect);
    }

    delete windowSystem;
}

//=============================================================================
// Test 10: Smart pointer usage (recommended pattern)
//=============================================================================

TEST_F(LeakTest, SmartPointerUsage) {
    // Demonstrate recommended pattern using smart pointers
    std::unique_ptr<platform::IWindowSystem> windowSystem(
        platform::createWindowSystem()
    );
    ASSERT_NE(windowSystem, nullptr);

    std::unique_ptr<platform::IInputInjector> inputInjector(
        platform::createInputInjector(windowSystem.get())
    );
    ASSERT_NE(inputInjector, nullptr);

    std::unique_ptr<platform::IInputHook> inputHook(
        platform::createInputHook()
    );
    ASSERT_NE(inputHook, nullptr);

    std::unique_ptr<platform::IInputDriver> inputDriver(
        platform::createInputDriver()
    );
    ASSERT_NE(inputDriver, nullptr);

    // Exercise operations
    windowSystem->getForegroundWindow();

    // Note: Not calling inputHook->install() since it may throw in test environments
    // The smart pointers automatically clean up when test exits
}

//=============================================================================
// Test 11: Stress test - many rapid allocations
//=============================================================================

TEST_F(LeakTest, StressTestRapidAllocations) {
    std::vector<platform::IWindowSystem*> windowSystems;
    windowSystems.reserve(50);

    // Allocate many window systems
    for (int i = 0; i < 50; ++i) {
        platform::IWindowSystem* ws = platform::createWindowSystem();
        ASSERT_NE(ws, nullptr);
        windowSystems.push_back(ws);
    }

    // Clean them all up
    for (auto* ws : windowSystems) {
        delete ws;
    }
}

} // namespace yamy::test

//=============================================================================
// Main entry point
//=============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
