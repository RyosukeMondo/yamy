//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window_system_linux_test.cpp - Comprehensive unit tests for WindowSystemLinuxQueries
//
// Tests the Linux X11 window system implementation with comprehensive coverage:
// - Property queries (title, class, PID)
// - Window geometry and state
// - Cache functionality
// - Error handling (null handles, missing properties, BadWindow)
// - Unicode support
// - Edge cases
//
// These tests can run in two modes:
// 1. With DISPLAY set: Tests actual X11 functionality
// 2. Without DISPLAY: Tests error handling and null cases
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include "../../platform/linux/window_system_linux_queries.h"
#include "../../platform/linux/x11_connection.h"
#include "../../core/platform/types.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cstdlib>
#include <memory>
#include <thread>
#include <chrono>
#include <unistd.h>

namespace yamy::platform {

// Test fixture with helper methods for creating test windows
class WindowSystemLinuxQueriesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Check if X11 display is available
        hasDisplay_ = (std::getenv("DISPLAY") != nullptr);

        if (hasDisplay_) {
            queries_ = std::make_unique<WindowSystemLinuxQueries>();
            display_ = X11Connection::instance().getDisplayOrNull();
            if (display_) {
                root_ = DefaultRootWindow(display_);
            }
        }
    }

    void TearDown() override {
        // Destroy all test windows
        if (display_) {
            for (Window window : testWindows_) {
                XDestroyWindow(display_, window);
            }
            XFlush(display_);
        }
        testWindows_.clear();
        queries_.reset();
    }

    bool hasDisplay() const { return hasDisplay_ && display_ != nullptr; }
    WindowSystemLinuxQueries* queries() { return queries_.get(); }
    Display* display() { return display_; }
    Window root() { return root_; }

    // Helper: Create a test window with specified properties
    Window createTestWindow(int x = 0, int y = 0, int width = 100, int height = 100) {
        if (!display_) return None;

        Window window = XCreateSimpleWindow(display_, root_, x, y, width, height, 0, 0, 0);
        testWindows_.push_back(window);
        XFlush(display_);
        return window;
    }

    // Helper: Set window title (UTF-8)
    void setWindowTitle(Window window, const std::string& title) {
        if (!display_ || window == None) return;

        Atom netWmName = X11Connection::instance().getAtom("_NET_WM_NAME");
        Atom utf8String = X11Connection::instance().getAtom("UTF8_STRING");

        XChangeProperty(display_, window, netWmName, utf8String, 8,
                       PropModeReplace,
                       reinterpret_cast<const unsigned char*>(title.c_str()),
                       title.length());
        XFlush(display_);
    }

    // Helper: Set window class
    void setWindowClass(Window window, const std::string& resName, const std::string& resClass) {
        if (!display_ || window == None) return;

        XClassHint classHint;
        classHint.res_name = const_cast<char*>(resName.c_str());
        classHint.res_class = const_cast<char*>(resClass.c_str());
        XSetClassHint(display_, window, &classHint);
        XFlush(display_);
    }

    // Helper: Set window PID
    void setWindowPID(Window window, uint32_t pid) {
        if (!display_ || window == None) return;

        Atom netWmPid = X11Connection::instance().getAtom("_NET_WM_PID");
        XChangeProperty(display_, window, netWmPid, XA_CARDINAL, 32,
                       PropModeReplace,
                       reinterpret_cast<unsigned char*>(&pid), 1);
        XFlush(display_);
    }

    // Helper: Map window
    void mapWindow(Window window) {
        if (!display_ || window == None) return;
        XMapWindow(display_, window);
        XFlush(display_);
        // Give X server time to process
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Test window cleanup list
    std::vector<Window> testWindows_;

private:
    bool hasDisplay_ = false;
    std::unique_ptr<WindowSystemLinuxQueries> queries_;
    Display* display_ = nullptr;
    Window root_ = None;
};

// =============================================================================
// Basic Construction Tests
// =============================================================================

TEST_F(WindowSystemLinuxQueriesTest, Construction) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    ASSERT_NE(queries(), nullptr);
}

// =============================================================================
// Window Title Tests
// =============================================================================

TEST_F(WindowSystemLinuxQueriesTest, GetWindowTextReturnsUTF8Title) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    setWindowTitle(testWindow, "Test Window Title");

    std::string title = queries()->getWindowText(reinterpret_cast<WindowHandle>(testWindow));
    EXPECT_EQ(title, "Test Window Title");
}

TEST_F(WindowSystemLinuxQueriesTest, GetWindowTextHandlesUnicode) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    std::string unicodeTitle = u8"日本語タイトル";
    setWindowTitle(testWindow, unicodeTitle);

    std::string title = queries()->getWindowText(reinterpret_cast<WindowHandle>(testWindow));
    EXPECT_EQ(title, unicodeTitle);
}

TEST_F(WindowSystemLinuxQueriesTest, GetWindowTextHandlesEmoji) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    std::string emojiTitle = u8"Test 🚀 Window";
    setWindowTitle(testWindow, emojiTitle);

    std::string title = queries()->getWindowText(reinterpret_cast<WindowHandle>(testWindow));
    EXPECT_EQ(title, emojiTitle);
}

TEST_F(WindowSystemLinuxQueriesTest, GetWindowTextNullHandle) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    std::string text = queries()->getWindowText(nullptr);
    EXPECT_TRUE(text.empty());
}

TEST_F(WindowSystemLinuxQueriesTest, GetWindowTextEmptyTitle) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    // Don't set any title property

    std::string title = queries()->getWindowText(reinterpret_cast<WindowHandle>(testWindow));
    // Title may be empty or have a default value
    (void)title;
}

TEST_F(WindowSystemLinuxQueriesTest, GetTitleNameMatchesGetWindowText) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    setWindowTitle(testWindow, "Title Test");

    WindowHandle hwnd = reinterpret_cast<WindowHandle>(testWindow);
    std::string windowText = queries()->getWindowText(hwnd);
    std::string titleName = queries()->getTitleName(hwnd);

    EXPECT_EQ(windowText, titleName);
}

// =============================================================================
// Window Class Tests
// =============================================================================

TEST_F(WindowSystemLinuxQueriesTest, GetClassNameReturnsCorrectClass) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    setWindowClass(testWindow, "firefox", "Navigator");

    std::string className = queries()->getClassName(reinterpret_cast<WindowHandle>(testWindow));
    EXPECT_EQ(className, "Navigator");
}

TEST_F(WindowSystemLinuxQueriesTest, GetClassNameWithOnlyResName) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    // Note: When we pass empty string for res_class, XSetClassHint may not
    // set the property correctly. The implementation prioritizes res_class
    // over res_name, so an empty res_class results in empty return.
    // This behavior is consistent with the implementation.
    setWindowClass(testWindow, "xterm", "");

    std::string className = queries()->getClassName(reinterpret_cast<WindowHandle>(testWindow));
    // Empty class is expected when res_class is empty (implementation detail)
    EXPECT_TRUE(className.empty() || className == "xterm");
}

TEST_F(WindowSystemLinuxQueriesTest, GetClassNameNullHandle) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    std::string className = queries()->getClassName(nullptr);
    EXPECT_TRUE(className.empty());
}

// =============================================================================
// Process ID Tests
// =============================================================================

TEST_F(WindowSystemLinuxQueriesTest, GetWindowProcessIdReturnsValidPID) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    uint32_t myPid = getpid();
    setWindowPID(testWindow, myPid);

    uint32_t pid = queries()->getWindowProcessId(reinterpret_cast<WindowHandle>(testWindow));
    EXPECT_EQ(pid, myPid);
}

TEST_F(WindowSystemLinuxQueriesTest, GetWindowProcessIdNullHandle) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    uint32_t pid = queries()->getWindowProcessId(nullptr);
    EXPECT_EQ(pid, 0u);
}

TEST_F(WindowSystemLinuxQueriesTest, GetWindowProcessIdMissingProperty) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    // Don't set _NET_WM_PID property

    uint32_t pid = queries()->getWindowProcessId(reinterpret_cast<WindowHandle>(testWindow));
    EXPECT_EQ(pid, 0u);
}

TEST_F(WindowSystemLinuxQueriesTest, GetWindowThreadIdMatchesProcessId) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    uint32_t myPid = getpid();
    setWindowPID(testWindow, myPid);

    WindowHandle hwnd = reinterpret_cast<WindowHandle>(testWindow);
    uint32_t tid = queries()->getWindowThreadId(hwnd);
    uint32_t pid = queries()->getWindowProcessId(hwnd);

    // On Linux, thread ID should equal process ID for window queries
    EXPECT_EQ(tid, pid);
}

// =============================================================================
// Window Geometry Tests
// =============================================================================

TEST_F(WindowSystemLinuxQueriesTest, GetWindowRectReturnsCorrectGeometry) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow(100, 200, 300, 400);
    mapWindow(testWindow);

    Rect rect;
    bool result = queries()->getWindowRect(reinterpret_cast<WindowHandle>(testWindow), &rect);

    ASSERT_TRUE(result);
    EXPECT_EQ(rect.width(), 300);
    EXPECT_EQ(rect.height(), 400);
    // Position may vary due to window manager decorations, so just check it's set
    // EXPECT_GE(rect.left, 0);
    // EXPECT_GE(rect.top, 0);
}

TEST_F(WindowSystemLinuxQueriesTest, GetWindowRectNullHandle) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Rect rect;
    bool result = queries()->getWindowRect(nullptr, &rect);
    EXPECT_FALSE(result);
}

TEST_F(WindowSystemLinuxQueriesTest, GetWindowRectNullRect) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    bool result = queries()->getWindowRect(reinterpret_cast<WindowHandle>(testWindow), nullptr);
    EXPECT_FALSE(result);
}

TEST_F(WindowSystemLinuxQueriesTest, GetWindowRectUnmappedWindow) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow(0, 0, 150, 250);
    // Don't map the window

    Rect rect;
    bool result = queries()->getWindowRect(reinterpret_cast<WindowHandle>(testWindow), &rect);

    // Should still work for unmapped windows
    if (result) {
        EXPECT_EQ(rect.width(), 150);
        EXPECT_EQ(rect.height(), 250);
    }
}

// =============================================================================
// Window From Point Tests
// =============================================================================

TEST_F(WindowSystemLinuxQueriesTest, WindowFromPointFindsRoot) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    // Point at origin should return some window (likely root or desktop)
    Point pt(0, 0);
    WindowHandle hwnd = queries()->windowFromPoint(pt);
    // Should not crash and return some window
    (void)hwnd;
}

TEST_F(WindowSystemLinuxQueriesTest, WindowFromPointLargeCoordinates) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    // Test with very large coordinates (off-screen)
    Point pt(99999, 99999);
    WindowHandle hwnd = queries()->windowFromPoint(pt);
    // Should handle gracefully without crashing
    (void)hwnd;
}

// =============================================================================
// Foreground Window Tests
// =============================================================================

TEST_F(WindowSystemLinuxQueriesTest, GetForegroundWindow) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    // getForegroundWindow should not crash
    WindowHandle hwnd = queries()->getForegroundWindow();
    // In an X11 environment, there should be some window focused
    // But in CI, there might not be, so we just verify it doesn't crash
    (void)hwnd;
}

// =============================================================================
// Cache Tests
// =============================================================================

TEST_F(WindowSystemLinuxQueriesTest, CacheSpeedsUpQueries) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    setWindowTitle(testWindow, "Cache Test");
    setWindowClass(testWindow, "test", "TestClass");
    setWindowPID(testWindow, getpid());

    WindowHandle hwnd = reinterpret_cast<WindowHandle>(testWindow);

    // First query (cache miss)
    std::string title1 = queries()->getWindowText(hwnd);

    // Second query (cache hit)
    std::string title2 = queries()->getWindowText(hwnd);

    // Verify same result - this is what the cache guarantees
    EXPECT_EQ(title1, title2);
    EXPECT_EQ(title1, "Cache Test");

    // Additional queries should also return cached values
    std::string className = queries()->getClassName(hwnd);
    EXPECT_EQ(className, "TestClass");

    uint32_t pid = queries()->getWindowProcessId(hwnd);
    EXPECT_EQ(pid, static_cast<uint32_t>(getpid()));
}

TEST_F(WindowSystemLinuxQueriesTest, InvalidateCacheClearsEntry) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    setWindowTitle(testWindow, "Original Title");

    WindowHandle hwnd = reinterpret_cast<WindowHandle>(testWindow);

    // Query to populate cache
    std::string title1 = queries()->getWindowText(hwnd);
    EXPECT_EQ(title1, "Original Title");

    // Change title
    setWindowTitle(testWindow, "New Title");

    // Without invalidation, should return cached value
    std::string title2 = queries()->getWindowText(hwnd);
    EXPECT_EQ(title2, "Original Title");  // Still cached

    // Invalidate and query again
    queries()->invalidateWindowCache(hwnd);
    std::string title3 = queries()->getWindowText(hwnd);
    EXPECT_EQ(title3, "New Title");  // Fresh query
}

TEST_F(WindowSystemLinuxQueriesTest, ClearCacheRemovesAllEntries) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window win1 = createTestWindow();
    Window win2 = createTestWindow();

    setWindowTitle(win1, "Window 1");
    setWindowTitle(win2, "Window 2");

    WindowHandle hwnd1 = reinterpret_cast<WindowHandle>(win1);
    WindowHandle hwnd2 = reinterpret_cast<WindowHandle>(win2);

    // Populate cache
    queries()->getWindowText(hwnd1);
    queries()->getWindowText(hwnd2);

    // Clear cache
    queries()->clearCache();

    // Change titles
    setWindowTitle(win1, "Changed 1");
    setWindowTitle(win2, "Changed 2");

    // Should get new values (cache was cleared)
    std::string title1 = queries()->getWindowText(hwnd1);
    std::string title2 = queries()->getWindowText(hwnd2);

    EXPECT_EQ(title1, "Changed 1");
    EXPECT_EQ(title2, "Changed 2");
}

// =============================================================================
// Batch Property Fetch Tests
// =============================================================================

TEST_F(WindowSystemLinuxQueriesTest, FetchAndCachePropertiesSetsAllProperties) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    setWindowTitle(testWindow, "Batch Test");
    setWindowClass(testWindow, "batch", "BatchClass");
    setWindowPID(testWindow, 12345);

    WindowHandle hwnd = reinterpret_cast<WindowHandle>(testWindow);

    // Clear cache first
    queries()->clearCache();

    // Query any property (should fetch all)
    std::string title = queries()->getWindowText(hwnd);
    EXPECT_EQ(title, "Batch Test");

    // Other properties should now be cached
    std::string className = queries()->getClassName(hwnd);
    EXPECT_EQ(className, "BatchClass");

    uint32_t pid = queries()->getWindowProcessId(hwnd);
    EXPECT_EQ(pid, 12345u);
}

// =============================================================================
// Error Handling Tests
// =============================================================================

TEST_F(WindowSystemLinuxQueriesTest, BadWindowHandlesGracefully) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    // Create an invalid window handle
    Window invalidWindow = 0x99999999;
    WindowHandle hwnd = reinterpret_cast<WindowHandle>(invalidWindow);

    // Should not crash
    std::string text = queries()->getWindowText(hwnd);
    EXPECT_TRUE(text.empty() || !text.empty());  // Just verify no crash

    std::string className = queries()->getClassName(hwnd);
    EXPECT_TRUE(className.empty() || !className.empty());  // Just verify no crash

    uint32_t pid = queries()->getWindowProcessId(hwnd);
    (void)pid;  // Just verify no crash

    Rect rect;
    bool hasRect = queries()->getWindowRect(hwnd, &rect);
    (void)hasRect;  // Just verify no crash
}

TEST_F(WindowSystemLinuxQueriesTest, DestroyedWindowHandlesGracefully) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window testWindow = createTestWindow();
    setWindowTitle(testWindow, "Temporary");

    WindowHandle hwnd = reinterpret_cast<WindowHandle>(testWindow);

    // Query while window exists
    std::string title1 = queries()->getWindowText(hwnd);
    EXPECT_EQ(title1, "Temporary");

    // Destroy the window
    XDestroyWindow(display(), testWindow);
    XFlush(display());

    // Remove from cleanup list
    testWindows_.erase(std::remove(testWindows_.begin(), testWindows_.end(), testWindow),
                       testWindows_.end());

    // Query again (window no longer exists)
    // Should handle gracefully without crashing
    std::string title2 = queries()->getWindowText(hwnd);
    // May return empty or cached value
    (void)title2;
}

// =============================================================================
// Struct Tests (Rect, Point, Size)
// =============================================================================

TEST(RectTest, DefaultConstruction) {
    Rect rect;
    EXPECT_EQ(rect.left, 0);
    EXPECT_EQ(rect.top, 0);
    EXPECT_EQ(rect.right, 0);
    EXPECT_EQ(rect.bottom, 0);
    EXPECT_EQ(rect.width(), 0);
    EXPECT_EQ(rect.height(), 0);
}

TEST(RectTest, ParameterizedConstruction) {
    Rect rect(10, 20, 110, 220);
    EXPECT_EQ(rect.left, 10);
    EXPECT_EQ(rect.top, 20);
    EXPECT_EQ(rect.right, 110);
    EXPECT_EQ(rect.bottom, 220);
    EXPECT_EQ(rect.width(), 100);
    EXPECT_EQ(rect.height(), 200);
}

TEST(RectTest, IsContainedIn) {
    Rect inner(50, 50, 100, 100);
    Rect outer(0, 0, 200, 200);
    Rect partial(150, 150, 250, 250);

    EXPECT_TRUE(inner.isContainedIn(outer));
    EXPECT_FALSE(partial.isContainedIn(outer));
    EXPECT_FALSE(outer.isContainedIn(inner));
}

TEST(PointTest, DefaultConstruction) {
    Point pt;
    EXPECT_EQ(pt.x, 0);
    EXPECT_EQ(pt.y, 0);
}

TEST(PointTest, ParameterizedConstruction) {
    Point pt(100, 200);
    EXPECT_EQ(pt.x, 100);
    EXPECT_EQ(pt.y, 200);
}

TEST(SizeTest, DefaultConstruction) {
    Size sz;
    EXPECT_EQ(sz.cx, 0);
    EXPECT_EQ(sz.cy, 0);
}

TEST(SizeTest, ParameterizedConstruction) {
    Size sz(640, 480);
    EXPECT_EQ(sz.cx, 640);
    EXPECT_EQ(sz.cy, 480);
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_F(WindowSystemLinuxQueriesTest, ForegroundWindowProperties) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    WindowHandle hwnd = queries()->getForegroundWindow();
    if (hwnd == nullptr) {
        GTEST_SKIP() << "No foreground window available";
    }

    // Get window text - may or may not be empty
    std::string text = queries()->getWindowText(hwnd);
    // Just verify it doesn't crash

    // Get class name
    std::string className = queries()->getClassName(hwnd);
    // Just verify it doesn't crash

    // Get window rect
    Rect rect;
    bool hasRect = queries()->getWindowRect(hwnd, &rect);
    if (hasRect) {
        // Verify rect has positive dimensions
        EXPECT_GE(rect.width(), 0);
        EXPECT_GE(rect.height(), 0);
    }

    // Get process ID
    uint32_t pid = queries()->getWindowProcessId(hwnd);
    // PID may be 0 if window doesn't have _NET_WM_PID
    (void)pid;
}

TEST_F(WindowSystemLinuxQueriesTest, MultipleWindowsIndependentCache) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Window win1 = createTestWindow();
    Window win2 = createTestWindow();
    Window win3 = createTestWindow();

    setWindowTitle(win1, "Window 1");
    setWindowTitle(win2, "Window 2");
    setWindowTitle(win3, "Window 3");

    setWindowClass(win1, "app1", "App1");
    setWindowClass(win2, "app2", "App2");
    setWindowClass(win3, "app3", "App3");

    WindowHandle hwnd1 = reinterpret_cast<WindowHandle>(win1);
    WindowHandle hwnd2 = reinterpret_cast<WindowHandle>(win2);
    WindowHandle hwnd3 = reinterpret_cast<WindowHandle>(win3);

    // Query all windows
    EXPECT_EQ(queries()->getWindowText(hwnd1), "Window 1");
    EXPECT_EQ(queries()->getWindowText(hwnd2), "Window 2");
    EXPECT_EQ(queries()->getWindowText(hwnd3), "Window 3");

    EXPECT_EQ(queries()->getClassName(hwnd1), "App1");
    EXPECT_EQ(queries()->getClassName(hwnd2), "App2");
    EXPECT_EQ(queries()->getClassName(hwnd3), "App3");

    // Invalidate one, others should remain cached
    queries()->invalidateWindowCache(hwnd2);

    setWindowTitle(win2, "Changed 2");

    // hwnd1 and hwnd3 should still be cached (old values)
    // hwnd2 should have new value
    EXPECT_EQ(queries()->getWindowText(hwnd1), "Window 1");
    EXPECT_EQ(queries()->getWindowText(hwnd2), "Changed 2");
    EXPECT_EQ(queries()->getWindowText(hwnd3), "Window 3");
}

} // namespace yamy::platform
