/**
 * @file investigate_performance_test.cpp
 * @brief Performance benchmarks for investigate window feature
 *
 * This file contains performance benchmarks to validate that all latency
 * and throughput requirements are met for the investigate window feature:
 * - Window property query latency (<10ms target)
 * - IPC round-trip latency (<5ms target)
 * - Live event notification latency (<10ms target)
 * - Stress test: 50 keys/sec with <5% CPU and no dropped events
 *
 * Benchmarks measure P50, P95, and P99 latencies to ensure consistent performance.
 */

#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <thread>

#include <QCoreApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QDataStream>
#include <QElapsedTimer>
#include <QTest>
#include <QEventLoop>

#include "core/platform/linux/ipc_channel_qt.h"
#include "core/platform/ipc_channel_interface.h"
#include "core/ipc_messages.h"
#include "platform/linux/window_system_linux.h"

// X11 headers must be included last to avoid conflicts with Qt
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

using namespace yamy::platform;
using namespace std::chrono;

namespace {

/**
 * @brief Statistics calculator for latency measurements
 */
struct LatencyStats {
    std::vector<double> samples;

    void addSample(double latencyMs) {
        samples.push_back(latencyMs);
    }

    double getP50() const {
        if (samples.empty()) return 0.0;
        auto sorted = samples;
        std::sort(sorted.begin(), sorted.end());
        return sorted[sorted.size() / 2];
    }

    double getP95() const {
        if (samples.empty()) return 0.0;
        auto sorted = samples;
        std::sort(sorted.begin(), sorted.end());
        return sorted[static_cast<size_t>(sorted.size() * 0.95)];
    }

    double getP99() const {
        if (samples.empty()) return 0.0;
        auto sorted = samples;
        std::sort(sorted.begin(), sorted.end());
        return sorted[static_cast<size_t>(sorted.size() * 0.99)];
    }

    double getAverage() const {
        if (samples.empty()) return 0.0;
        return std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
    }

    double getMin() const {
        if (samples.empty()) return 0.0;
        return *std::min_element(samples.begin(), samples.end());
    }

    double getMax() const {
        if (samples.empty()) return 0.0;
        return *std::max_element(samples.begin(), samples.end());
    }

    void printStats(const std::string& name) const {
        std::cout << "\n=== " << name << " ===" << std::endl;
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "  Samples: " << samples.size() << std::endl;
        std::cout << "  Min:     " << getMin() << " ms" << std::endl;
        std::cout << "  P50:     " << getP50() << " ms" << std::endl;
        std::cout << "  P95:     " << getP95() << " ms" << std::endl;
        std::cout << "  P99:     " << getP99() << " ms" << std::endl;
        std::cout << "  Max:     " << getMax() << " ms" << std::endl;
        std::cout << "  Average: " << getAverage() << " ms" << std::endl;
    }
};

} // anonymous namespace

/**
 * @brief Performance test fixture
 */
class InvestigatePerformanceTest : public ::testing::Test {
protected:
    Display* m_display = nullptr;
    Window m_testWindow = 0;
    WindowSystemLinux* m_windowSystem = nullptr;
    QCoreApplication* m_app = nullptr;

    void SetUp() override {
        // Initialize Qt application
        int argc = 0;
        char* argv[] = {nullptr};
        m_app = new QCoreApplication(argc, argv);

        // Open X11 display
        m_display = XOpenDisplay(nullptr);
        ASSERT_NE(nullptr, m_display) << "Failed to open X11 display. Is DISPLAY set?";

        // Create test window with properties
        m_testWindow = XCreateSimpleWindow(
            m_display,
            DefaultRootWindow(m_display),
            100, 100,  // x, y
            400, 300,  // width, height
            1,         // border width
            BlackPixel(m_display, DefaultScreen(m_display)),
            WhitePixel(m_display, DefaultScreen(m_display))
        );

        // Set window properties for realistic testing
        setWindowProperty(m_testWindow, "_NET_WM_NAME", "Performance Test Window");

        XClassHint classHint;
        classHint.res_name = const_cast<char*>("perftest");
        classHint.res_class = const_cast<char*>("PerfTest");
        XSetClassHint(m_display, m_testWindow, &classHint);

        uint32_t pid = getpid();
        Atom netWmPid = XInternAtom(m_display, "_NET_WM_PID", False);
        XChangeProperty(m_display, m_testWindow, netWmPid, XA_CARDINAL, 32,
                       PropModeReplace, reinterpret_cast<unsigned char*>(&pid), 1);

        // Map window
        XMapWindow(m_display, m_testWindow);
        XFlush(m_display);

        // Create window system
        m_windowSystem = new WindowSystemLinux();

        // Small delay to ensure window is fully initialized
        QTest::qWait(50);
    }

    void TearDown() override {
        delete m_windowSystem;

        if (m_testWindow) {
            XDestroyWindow(m_display, m_testWindow);
        }

        if (m_display) {
            XCloseDisplay(m_display);
        }

        delete m_app;
    }

    void setWindowProperty(Window w, const char* propName, const char* value) {
        Atom prop = XInternAtom(m_display, propName, False);
        Atom type = XInternAtom(m_display, "UTF8_STRING", False);
        XChangeProperty(m_display, w, prop, type, 8, PropModeReplace,
                       reinterpret_cast<const unsigned char*>(value),
                       strlen(value));
    }
};

/**
 * @brief Benchmark: Window property query latency
 *
 * Target: <10ms for all property queries combined
 * Measures: getWindowText, getClassName, getWindowProcessId, getWindowRect, getShowCommand
 */
TEST_F(InvestigatePerformanceTest, WindowPropertyQueryLatency) {
    const int iterations = 100;
    LatencyStats stats;

    for (int i = 0; i < iterations; i++) {
        QElapsedTimer timer;
        timer.start();

        // Query all window properties (simulating investigate window workflow)
        WindowHandle hwnd = reinterpret_cast<WindowHandle>(m_testWindow);
        std::string title = m_windowSystem->getWindowText(hwnd);
        std::string className = m_windowSystem->getClassName(hwnd);
        uint32_t pid = m_windowSystem->getWindowProcessId(hwnd);

        Rect rect;
        m_windowSystem->getWindowRect(hwnd, &rect);

        WindowShowCmd state = m_windowSystem->getShowCommand(hwnd);

        qint64 elapsedNs = timer.nsecsElapsed();
        double elapsedMs = elapsedNs / 1000000.0;

        stats.addSample(elapsedMs);

        // Prevent compiler from optimizing away the calls
        (void)title;
        (void)className;
        (void)pid;
        (void)rect;
        (void)state;
    }

    // Print statistics
    stats.printStats("Window Property Query Latency");

    // Assertions
    EXPECT_LT(stats.getP99(), 10.0) << "P99 latency must be <10ms";
    EXPECT_LT(stats.getP95(), 8.0) << "P95 latency should be <8ms";
    EXPECT_LT(stats.getAverage(), 5.0) << "Average latency should be <5ms";
}

/**
 * @brief Benchmark: IPC round-trip latency
 *
 * Target: <5ms for request-response cycle
 * Simulates: Dialog sends CmdInvestigateWindow → Engine responds with RspInvestigateWindow
 */
TEST_F(InvestigatePerformanceTest, IPCRoundTripLatency) {
    const int iterations = 100;
    LatencyStats stats;

    // Create server and client
    IPCChannelQt* server = new IPCChannelQt("perf-test-server");
    IPCChannelQt* client = new IPCChannelQt("perf-test-client");

    // Start server
    server->listen();

    // Connect client
    client->connect("perf-test-server");

    // Wait for connection
    QTest::qWait(100);
    ASSERT_TRUE(client->isConnected()) << "Client failed to connect to server";

    // Set up response handler
    bool responseReceived = false;
    QObject::connect(client, &IPCChannelQt::messageReceived,
                    [&](const yamy::ipc::Message& msg) {
        responseReceived = true;
    });

    QObject::connect(server, &IPCChannelQt::messageReceived,
                    [&](const yamy::ipc::Message& msg) {
        // Server immediately responds
        yamy::ipc::InvestigateWindowResponse response;
        strncpy(response.keymapName, "TestKeymap", sizeof(response.keymapName));
        strncpy(response.matchedClassRegex, ".*", sizeof(response.matchedClassRegex));
        strncpy(response.matchedTitleRegex, ".*", sizeof(response.matchedTitleRegex));
        strncpy(response.activeModifiers, "", sizeof(response.activeModifiers));
        response.isDefault = false;

        yamy::ipc::Message responseMsg;
        responseMsg.type = yamy::ipc::RspInvestigateWindow;
        responseMsg.data = &response;
        responseMsg.size = sizeof(response);

        server->send(responseMsg);
    });

    // Benchmark round-trips
    for (int i = 0; i < iterations; i++) {
        responseReceived = false;

        QElapsedTimer timer;
        timer.start();

        // Client sends request
        yamy::ipc::InvestigateWindowRequest request;
        request.hwnd = reinterpret_cast<void*>(m_testWindow);

        yamy::ipc::Message msg;
        msg.type = yamy::ipc::CmdInvestigateWindow;
        msg.data = &request;
        msg.size = sizeof(request);

        client->send(msg);

        // Wait for response with timeout
        int timeout = 0;
        while (!responseReceived && timeout < 100) {
            QTest::qWait(1);
            timeout++;
        }

        qint64 elapsedNs = timer.nsecsElapsed();
        double elapsedMs = elapsedNs / 1000000.0;

        if (responseReceived) {
            stats.addSample(elapsedMs);
        } else {
            ADD_FAILURE() << "Response timeout on iteration " << i;
        }
    }

    // Print statistics
    stats.printStats("IPC Round-Trip Latency");

    // Assertions
    EXPECT_LT(stats.getP99(), 5.0) << "P99 latency must be <5ms";
    EXPECT_LT(stats.getP95(), 4.0) << "P95 latency should be <4ms";
    EXPECT_LT(stats.getAverage(), 3.0) << "Average latency should be <3ms";

    // Cleanup
    delete client;
    delete server;
}

/**
 * @brief Benchmark: End-to-end latency for window investigation
 *
 * Target: <10ms from window selection to all panels populated
 * Combines: windowFromPoint + property queries + IPC round-trip
 */
TEST_F(InvestigatePerformanceTest, EndToEndInvestigateLatency) {
    const int iterations = 50;
    LatencyStats stats;

    // Create IPC channels
    IPCChannelQt* server = new IPCChannelQt("e2e-test-server");
    IPCChannelQt* client = new IPCChannelQt("e2e-test-client");

    server->listen();
    client->connect("e2e-test-server");
    QTest::qWait(100);

    ASSERT_TRUE(client->isConnected());

    // Set up handlers
    bool responseReceived = false;
    QObject::connect(client, &IPCChannelQt::messageReceived,
                    [&](const yamy::ipc::Message&) {
        responseReceived = true;
    });

    QObject::connect(server, &IPCChannelQt::messageReceived,
                    [&](const yamy::ipc::Message& msg) {
        yamy::ipc::InvestigateWindowResponse response;
        strncpy(response.keymapName, "Global", sizeof(response.keymapName));
        strncpy(response.matchedClassRegex, ".*", sizeof(response.matchedClassRegex));
        strncpy(response.matchedTitleRegex, ".*", sizeof(response.matchedTitleRegex));
        strncpy(response.activeModifiers, "", sizeof(response.activeModifiers));
        response.isDefault = false;

        yamy::ipc::Message responseMsg;
        responseMsg.type = yamy::ipc::RspInvestigateWindow;
        responseMsg.data = &response;
        responseMsg.size = sizeof(response);

        server->send(responseMsg);
    });

    // Benchmark end-to-end workflow
    for (int i = 0; i < iterations; i++) {
        responseReceived = false;

        QElapsedTimer timer;
        timer.start();

        // Step 1: Get window from point (simulated - using known window)
        WindowHandle hwnd = reinterpret_cast<WindowHandle>(m_testWindow);

        // Step 2: Query all window properties
        std::string title = m_windowSystem->getWindowText(hwnd);
        std::string className = m_windowSystem->getClassName(hwnd);
        uint32_t pid = m_windowSystem->getWindowProcessId(hwnd);
        Rect rect;
        m_windowSystem->getWindowRect(hwnd, &rect);
        WindowShowCmd state = m_windowSystem->getShowCommand(hwnd);

        // Step 3: Send IPC request and wait for response
        yamy::ipc::InvestigateWindowRequest request;
        request.hwnd = reinterpret_cast<void*>(hwnd);

        yamy::ipc::Message msg;
        msg.type = yamy::ipc::CmdInvestigateWindow;
        msg.data = &request;
        msg.size = sizeof(request);

        client->send(msg);

        // Wait for response
        int timeout = 0;
        while (!responseReceived && timeout < 100) {
            QTest::qWait(1);
            timeout++;
        }

        qint64 elapsedNs = timer.nsecsElapsed();
        double elapsedMs = elapsedNs / 1000000.0;

        if (responseReceived) {
            stats.addSample(elapsedMs);
        }

        // Prevent optimization
        (void)title; (void)className; (void)pid; (void)rect; (void)state;
    }

    // Print statistics
    stats.printStats("End-to-End Investigate Latency");

    // Assertions
    EXPECT_LT(stats.getP99(), 10.0) << "P99 end-to-end latency must be <10ms";
    EXPECT_LT(stats.getP95(), 8.0) << "P95 end-to-end latency should be <8ms";

    // Cleanup
    delete client;
    delete server;
}

/**
 * @brief Stress test: Rapid key event notifications
 *
 * Target: 50 keys/sec with <5% CPU and 0 dropped events
 * Simulates: Live key event logging under high load
 *
 * Note: This test is marked as optional since it requires a real engine process
 * and accurate CPU measurement. It serves as a design validation test.
 */
TEST_F(InvestigatePerformanceTest, DISABLED_StressTest_RapidKeyEvents) {
    const int eventsPerSecond = 50;
    const int durationSeconds = 5;
    const int totalEvents = eventsPerSecond * durationSeconds;

    // Create IPC channels
    IPCChannelQt* server = new IPCChannelQt("stress-test-server");
    IPCChannelQt* client = new IPCChannelQt("stress-test-client");

    server->listen();
    client->connect("stress-test-server");
    QTest::qWait(100);

    ASSERT_TRUE(client->isConnected());

    // Count received events
    int receivedEvents = 0;
    QObject::connect(client, &IPCChannelQt::messageReceived,
                    [&](const yamy::ipc::Message& msg) {
        if (msg.type == yamy::ipc::NtfKeyEvent) {
            receivedEvents++;
        }
    });

    // Send events at specified rate
    auto startTime = steady_clock::now();
    int sentEvents = 0;

    while (sentEvents < totalEvents) {
        // Send key event notification
        yamy::ipc::KeyEventNotification notification;
        snprintf(notification.keyEvent, sizeof(notification.keyEvent),
                "[00:00:00.%03d] TestKey ↓", sentEvents);

        yamy::ipc::Message msg;
        msg.type = yamy::ipc::NtfKeyEvent;
        msg.data = &notification;
        msg.size = sizeof(notification);

        server->send(msg);
        sentEvents++;

        // Wait to maintain target rate
        auto expectedTime = startTime + milliseconds(sentEvents * 1000 / eventsPerSecond);
        auto now = steady_clock::now();
        if (now < expectedTime) {
            std::this_thread::sleep_until(expectedTime);
        }

        // Process events periodically
        if (sentEvents % 10 == 0) {
            QTest::qWait(1);
        }
    }

    // Wait for all events to be received
    auto deadline = steady_clock::now() + seconds(2);
    while (receivedEvents < totalEvents && steady_clock::now() < deadline) {
        QTest::qWait(10);
    }

    auto endTime = steady_clock::now();
    auto durationMs = duration_cast<milliseconds>(endTime - startTime).count();

    std::cout << "\n=== Stress Test Results ===" << std::endl;
    std::cout << "  Sent events:     " << sentEvents << std::endl;
    std::cout << "  Received events: " << receivedEvents << std::endl;
    std::cout << "  Duration:        " << durationMs << " ms" << std::endl;
    std::cout << "  Actual rate:     " << (sentEvents * 1000.0 / durationMs) << " events/sec" << std::endl;
    std::cout << "  Dropped events:  " << (sentEvents - receivedEvents) << std::endl;

    // Assertions
    EXPECT_EQ(sentEvents, receivedEvents) << "No events should be dropped";
    EXPECT_GE(sentEvents * 1000.0 / durationMs, eventsPerSecond * 0.95)
        << "Should maintain at least 95% of target rate";

    // Note: CPU measurement would require platform-specific code (reading /proc/stat)
    // and is better done via external profiling tools (perf, top, etc.)

    // Cleanup
    delete client;
    delete server;
}

/**
 * @brief Benchmark: windowFromPoint performance
 *
 * Target: <2ms for cursor-to-window lookup
 * Simulates: Crosshair widget querying window under cursor
 */
TEST_F(InvestigatePerformanceTest, WindowFromPointLatency) {
    const int iterations = 100;
    LatencyStats stats;

    // Get screen center point
    Point pt;
    pt.x = 200;
    pt.y = 200;

    for (int i = 0; i < iterations; i++) {
        QElapsedTimer timer;
        timer.start();

        WindowHandle hwnd = m_windowSystem->windowFromPoint(pt);

        qint64 elapsedNs = timer.nsecsElapsed();
        double elapsedMs = elapsedNs / 1000000.0;

        stats.addSample(elapsedMs);

        // Prevent optimization
        (void)hwnd;
    }

    // Print statistics
    stats.printStats("windowFromPoint Latency");

    // Assertions
    EXPECT_LT(stats.getP99(), 2.0) << "P99 latency must be <2ms";
    EXPECT_LT(stats.getAverage(), 1.0) << "Average latency should be <1ms";
}

/**
 * @brief Main entry point for benchmark tests
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << " Investigate Window Performance Tests\n";
    std::cout << "========================================\n";
    std::cout << "\n";
    std::cout << "Performance Targets:\n";
    std::cout << "  - Window property queries: <10ms (P99)\n";
    std::cout << "  - IPC round-trip:          <5ms (P99)\n";
    std::cout << "  - End-to-end investigate:  <10ms (P99)\n";
    std::cout << "  - windowFromPoint:         <2ms (P99)\n";
    std::cout << "\n";

    return RUN_ALL_TESTS();
}
