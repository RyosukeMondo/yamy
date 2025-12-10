// Template for testing individual tracks
// Copy and modify for each track

#include <iostream>
#include "../../core/platform/types.h"

// Include the track header you're testing
// #include "window_system_linux_queries.h"

using namespace yamy::platform;

int main() {
    std::cout << "=== Track Test Harness ===" << std::endl;
    std::cout << "Testing: [Track Name]" << std::endl;
    std::cout << std::endl;

    // Create instance
    // WindowSystemLinuxQueries queries;

    // Run tests
    // Example:
    // auto hwnd = queries.getForegroundWindow();
    // if (hwnd) {
    //     std::cout << "✅ getForegroundWindow works" << std::endl;
    //     std::cout << "   Window handle: " << hwnd << std::endl;
    // } else {
    //     std::cout << "❌ getForegroundWindow failed" << std::endl;
    // }

    std::cout << std::endl;
    std::cout << "=== Test Complete ===" << std::endl;
    return 0;
}

/* Build instructions:
 *
 * g++ -o test_trackN test_trackN.cpp \
 *     src/platform/linux/[track_file].cpp \
 *     -lX11 -lpthread -I src/
 *
 * ./test_trackN
 */
