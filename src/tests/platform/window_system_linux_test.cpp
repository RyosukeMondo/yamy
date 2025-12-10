//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window_system_linux_test.cpp - Unit tests for WindowSystemLinuxQueries
//
// Tests the Linux X11 window system implementation.
// These tests can run in two modes:
// 1. With DISPLAY set: Tests actual X11 functionality
// 2. Without DISPLAY: Tests error handling and null cases
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gtest/gtest.h>
#include "../../platform/linux/window_system_linux_queries.h"
#include "../../core/platform/types.h"
#include <cstdlib>

namespace yamy::platform {

class WindowSystemLinuxQueriesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Check if X11 display is available
        hasDisplay_ = (std::getenv("DISPLAY") != nullptr);

        if (hasDisplay_) {
            queries_ = std::make_unique<WindowSystemLinuxQueries>();
        }
    }

    void TearDown() override {
        queries_.reset();
    }

    bool hasDisplay() const { return hasDisplay_; }
    WindowSystemLinuxQueries* queries() { return queries_.get(); }

private:
    bool hasDisplay_ = false;
    std::unique_ptr<WindowSystemLinuxQueries> queries_;
};

// Test that WindowSystemLinuxQueries can be constructed
TEST_F(WindowSystemLinuxQueriesTest, Construction) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    ASSERT_NE(queries(), nullptr);
}

// Test getForegroundWindow returns a handle (may be nullptr in headless)
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

// Test getWindowText with null handle
TEST_F(WindowSystemLinuxQueriesTest, GetWindowTextNullHandle) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    std::string text = queries()->getWindowText(nullptr);
    EXPECT_TRUE(text.empty());
}

// Test getClassName with null handle
TEST_F(WindowSystemLinuxQueriesTest, GetClassNameNullHandle) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    std::string className = queries()->getClassName(nullptr);
    EXPECT_TRUE(className.empty());
}

// Test getTitleName with null handle
TEST_F(WindowSystemLinuxQueriesTest, GetTitleNameNullHandle) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    std::string title = queries()->getTitleName(nullptr);
    EXPECT_TRUE(title.empty());
}

// Test getWindowProcessId with null handle
TEST_F(WindowSystemLinuxQueriesTest, GetWindowProcessIdNullHandle) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    uint32_t pid = queries()->getWindowProcessId(nullptr);
    EXPECT_EQ(pid, 0u);
}

// Test getWindowThreadId with null handle
TEST_F(WindowSystemLinuxQueriesTest, GetWindowThreadIdNullHandle) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    uint32_t tid = queries()->getWindowThreadId(nullptr);
    EXPECT_EQ(tid, 0u);
}

// Test getWindowRect with null handle
TEST_F(WindowSystemLinuxQueriesTest, GetWindowRectNullHandle) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    Rect rect;
    bool result = queries()->getWindowRect(nullptr, &rect);
    EXPECT_FALSE(result);
}

// Test getWindowRect with null rect pointer
TEST_F(WindowSystemLinuxQueriesTest, GetWindowRectNullRect) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    WindowHandle hwnd = queries()->getForegroundWindow();
    bool result = queries()->getWindowRect(hwnd, nullptr);
    EXPECT_FALSE(result);
}

// Test windowFromPoint
TEST_F(WindowSystemLinuxQueriesTest, WindowFromPoint) {
    if (!hasDisplay()) {
        GTEST_SKIP() << "No X11 display available";
    }

    // Test with origin point - should return root window or a window
    Point pt(0, 0);
    WindowHandle hwnd = queries()->windowFromPoint(pt);
    // In a real X11 environment, there should be a window at origin
    (void)hwnd;
}

// Test querying foreground window properties
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

// Test Rect struct
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

// Test Point struct
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

// Test Size struct
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

} // namespace yamy::platform
