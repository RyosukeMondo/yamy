//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// regression_suite.cpp - Comprehensive regression test suite for Linux platform
//
// This file aggregates all unit tests, integration tests, and platform tests
// into a single regression suite that runs in CI on every commit.
//
// Usage:
//   cmake -B build -DBUILD_REGRESSION_TESTS=ON -DBUILD_LINUX_STUB=ON
//   cmake --build build --target yamy_regression_test
//   xvfb-run -a ./build/bin/yamy_regression_test
//
// Coverage targets: >= 80% for platform code
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include <cstdlib>
#include <iostream>

//=============================================================================
// Test filter for CI environments
//=============================================================================

namespace {
    bool isCI() {
        return std::getenv("CI") != nullptr ||
               std::getenv("GITHUB_ACTIONS") != nullptr;
    }

    bool hasDisplay() {
        return std::getenv("DISPLAY") != nullptr;
    }
}

//=============================================================================
// Main entry point - Regression test runner
//=============================================================================

int main(int argc, char** argv) {
    std::cout << "=== YAMY Linux Regression Test Suite ===" << std::endl;
    std::cout << "Environment:" << std::endl;
    std::cout << "  CI: " << (isCI() ? "yes" : "no") << std::endl;
    std::cout << "  DISPLAY: " << (hasDisplay() ? std::getenv("DISPLAY") : "(none)") << std::endl;
    std::cout << std::endl;

    ::testing::InitGoogleTest(&argc, argv);

    // Set test filter to exclude known flaky tests in CI if needed
    if (isCI()) {
        // In CI, we may need to filter out tests that require real hardware
        // For now, run all tests - they should handle missing hardware gracefully
        std::cout << "[CI] Running in CI mode - tests will gracefully handle missing hardware" << std::endl;
    }

    // Run all tests
    int result = RUN_ALL_TESTS();

    std::cout << std::endl;
    std::cout << "=== Regression Test Suite Complete ===" << std::endl;

    // Flush all output streams before exit
    std::cout.flush();
    std::cerr.flush();

    // Use _exit() to avoid static destruction order issues.
    // The regression test links against yamy_core which has several singletons
    // (ConfigManager, SessionManager, Logger, etc.) whose destruction order
    // relative to google test's static parameterized test infrastructure can
    // trigger std::bad_variant_access during cleanup. Since all test results
    // are already reported by this point, skipping static destructors is safe.
    _exit(result);
}
