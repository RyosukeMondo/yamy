/**
 * @file tray_icon_test.cpp
 * @brief Comprehensive tests for TrayIconQt notification handling
 *
 * Tests cover:
 * - handleEngineMessage for all message types
 * - Icon state changes
 * - Tooltip updates
 * - Error notification display
 */

#include <gtest/gtest.h>
#include <QApplication>
#include <QTest>
#include <QString>

#include "ui/qt/tray_icon_qt.h"
#include "core/platform/ipc_defs.h"

// =============================================================================
// Test Fixture Setup
// =============================================================================

class TrayIconTest : public ::testing::Test {
protected:
    static QApplication* app;

    static void SetUpTestSuite() {
        if (!QApplication::instance()) {
            int argc = 0;
            app = new QApplication(argc, nullptr);
        }
    }

    void SetUp() override {
        // Create tray icon without engine for testing
        trayIcon = new TrayIconQt(nullptr);
    }

    void TearDown() override {
        delete trayIcon;
        trayIcon = nullptr;
        QApplication::processEvents();
    }

    TrayIconQt* trayIcon = nullptr;
};

QApplication* TrayIconTest::app = nullptr;

// =============================================================================
// Engine State Notification Tests
// =============================================================================

TEST_F(TrayIconTest, EngineStartingUpdatesIconAndTooltip) {
    trayIcon->handleEngineMessage(yamy::MessageType::EngineStarting, "");
    QApplication::processEvents();

    EXPECT_EQ(trayIcon->toolTip(), "YAMY - Starting...");
    EXPECT_FALSE(trayIcon->icon().isNull()) << "Icon should be set";
}

TEST_F(TrayIconTest, EngineStartedUpdatesIconAndTooltip) {
    trayIcon->handleEngineMessage(yamy::MessageType::EngineStarted, "");
    QApplication::processEvents();

    EXPECT_EQ(trayIcon->toolTip(), "YAMY - Running");
    EXPECT_FALSE(trayIcon->icon().isNull()) << "Icon should be set";
}

TEST_F(TrayIconTest, EngineStoppedUpdatesIconAndTooltip) {
    trayIcon->handleEngineMessage(yamy::MessageType::EngineStopped, "");
    QApplication::processEvents();

    EXPECT_EQ(trayIcon->toolTip(), "YAMY - Stopped");
    EXPECT_FALSE(trayIcon->icon().isNull()) << "Icon should be set";
}

TEST_F(TrayIconTest, EngineStoppingUpdatesIconAndTooltip) {
    trayIcon->handleEngineMessage(yamy::MessageType::EngineStopping, "");
    QApplication::processEvents();

    EXPECT_EQ(trayIcon->toolTip(), "YAMY - Stopping...");
    EXPECT_FALSE(trayIcon->icon().isNull()) << "Icon should be set";
}

TEST_F(TrayIconTest, EngineErrorShowsNotification) {
    QString errorMessage = "Test error message";
    trayIcon->handleEngineMessage(yamy::MessageType::EngineError, errorMessage);
    QApplication::processEvents();

    EXPECT_TRUE(trayIcon->toolTip().contains("Error"));
    EXPECT_TRUE(trayIcon->toolTip().contains(errorMessage));
}

TEST_F(TrayIconTest, EngineErrorWithEmptyMessageShowsDefault) {
    trayIcon->handleEngineMessage(yamy::MessageType::EngineError, "");
    QApplication::processEvents();

    EXPECT_TRUE(trayIcon->toolTip().contains("Error"));
}

// =============================================================================
// Config Notification Tests
// =============================================================================

TEST_F(TrayIconTest, ConfigLoadingUpdatesTooltip) {
    QString configPath = "/path/to/config.mayu";
    trayIcon->handleEngineMessage(yamy::MessageType::ConfigLoading, configPath);
    QApplication::processEvents();

    EXPECT_TRUE(trayIcon->toolTip().contains("Loading"));
    EXPECT_TRUE(trayIcon->toolTip().contains(configPath));
}

TEST_F(TrayIconTest, ConfigLoadedUpdatesTooltip) {
    QString configName = "work.mayu";
    trayIcon->handleEngineMessage(yamy::MessageType::ConfigLoaded, configName);
    QApplication::processEvents();

    EXPECT_TRUE(trayIcon->toolTip().contains(configName));
}

TEST_F(TrayIconTest, ConfigLoadedWithEmptyNameShowsRunning) {
    trayIcon->handleEngineMessage(yamy::MessageType::ConfigLoaded, "");
    QApplication::processEvents();

    EXPECT_EQ(trayIcon->toolTip(), "YAMY - Running");
}

TEST_F(TrayIconTest, ConfigErrorUpdatesTooltip) {
    QString errorMessage = "Config parse error";
    trayIcon->handleEngineMessage(yamy::MessageType::ConfigError, errorMessage);
    QApplication::processEvents();

    EXPECT_TRUE(trayIcon->toolTip().contains("Error"));
    EXPECT_TRUE(trayIcon->toolTip().contains(errorMessage));
}

// =============================================================================
// Runtime Event Tests
// =============================================================================

TEST_F(TrayIconTest, KeymapSwitchedUpdatesTooltip) {
    // First set a config name
    trayIcon->handleEngineMessage(yamy::MessageType::ConfigLoaded, "work.mayu");
    QApplication::processEvents();

    // Then switch keymap
    QString keymapName = "vim-mode";
    trayIcon->handleEngineMessage(yamy::MessageType::KeymapSwitched, keymapName);
    QApplication::processEvents();

    EXPECT_TRUE(trayIcon->toolTip().contains(keymapName));
}

TEST_F(TrayIconTest, KeymapSwitchedWithNoConfigName) {
    // Switch keymap without config loaded
    QString keymapName = "default";
    trayIcon->handleEngineMessage(yamy::MessageType::KeymapSwitched, keymapName);
    QApplication::processEvents();

    EXPECT_TRUE(trayIcon->toolTip().contains(keymapName));
}

TEST_F(TrayIconTest, FocusChangedDoesNotChangeIcon) {
    // Get current icon
    trayIcon->handleEngineMessage(yamy::MessageType::EngineStarted, "");
    QApplication::processEvents();
    QString tooltipBefore = trayIcon->toolTip();

    // Focus change should not update icon significantly
    trayIcon->handleEngineMessage(yamy::MessageType::FocusChanged, "New Window");
    QApplication::processEvents();

    // Tooltip should not have changed drastically
    EXPECT_EQ(trayIcon->toolTip(), tooltipBefore);
}

TEST_F(TrayIconTest, ModifierChangedDoesNotChangeIcon) {
    trayIcon->handleEngineMessage(yamy::MessageType::EngineStarted, "");
    QApplication::processEvents();
    QString tooltipBefore = trayIcon->toolTip();

    trayIcon->handleEngineMessage(yamy::MessageType::ModifierChanged, "Ctrl+Shift");
    QApplication::processEvents();

    EXPECT_EQ(trayIcon->toolTip(), tooltipBefore);
}

// =============================================================================
// Performance Metrics Tests
// =============================================================================

TEST_F(TrayIconTest, LatencyReportDoesNotChangeIcon) {
    trayIcon->handleEngineMessage(yamy::MessageType::EngineStarted, "");
    QApplication::processEvents();
    QString tooltipBefore = trayIcon->toolTip();

    trayIcon->handleEngineMessage(yamy::MessageType::LatencyReport, "P95: 1.2ms");
    QApplication::processEvents();

    // Performance metrics should not change tooltip
    EXPECT_EQ(trayIcon->toolTip(), tooltipBefore);
}

TEST_F(TrayIconTest, CpuUsageReportDoesNotChangeIcon) {
    trayIcon->handleEngineMessage(yamy::MessageType::EngineStarted, "");
    QApplication::processEvents();
    QString tooltipBefore = trayIcon->toolTip();

    trayIcon->handleEngineMessage(yamy::MessageType::CpuUsageReport, "CPU: 5%");
    QApplication::processEvents();

    EXPECT_EQ(trayIcon->toolTip(), tooltipBefore);
}

// =============================================================================
// State Transition Tests
// =============================================================================

TEST_F(TrayIconTest, StartingToStartedTransition) {
    trayIcon->handleEngineMessage(yamy::MessageType::EngineStarting, "");
    QApplication::processEvents();
    EXPECT_EQ(trayIcon->toolTip(), "YAMY - Starting...");

    trayIcon->handleEngineMessage(yamy::MessageType::EngineStarted, "");
    QApplication::processEvents();
    EXPECT_EQ(trayIcon->toolTip(), "YAMY - Running");
}

TEST_F(TrayIconTest, RunningToStoppedTransition) {
    trayIcon->handleEngineMessage(yamy::MessageType::EngineStarted, "");
    QApplication::processEvents();
    EXPECT_EQ(trayIcon->toolTip(), "YAMY - Running");

    trayIcon->handleEngineMessage(yamy::MessageType::EngineStopping, "");
    QApplication::processEvents();
    EXPECT_EQ(trayIcon->toolTip(), "YAMY - Stopping...");

    trayIcon->handleEngineMessage(yamy::MessageType::EngineStopped, "");
    QApplication::processEvents();
    EXPECT_EQ(trayIcon->toolTip(), "YAMY - Stopped");
}

TEST_F(TrayIconTest, ConfigLoadedAfterEngineStarted) {
    trayIcon->handleEngineMessage(yamy::MessageType::EngineStarted, "");
    QApplication::processEvents();

    trayIcon->handleEngineMessage(yamy::MessageType::ConfigLoaded, "gaming.mayu");
    QApplication::processEvents();

    EXPECT_EQ(trayIcon->toolTip(), "YAMY - gaming.mayu");
}

TEST_F(TrayIconTest, EngineStartedAfterConfigLoaded) {
    // Config loaded message comes first (unusual but possible)
    trayIcon->handleEngineMessage(yamy::MessageType::ConfigLoaded, "work.mayu");
    QApplication::processEvents();

    trayIcon->handleEngineMessage(yamy::MessageType::EngineStarted, "");
    QApplication::processEvents();

    // Should show running with config name
    EXPECT_TRUE(trayIcon->toolTip().contains("Running") ||
                trayIcon->toolTip().contains("work.mayu"));
}

// =============================================================================
// Icon State Tests
// =============================================================================

TEST_F(TrayIconTest, InitialIconIsNotNull) {
    EXPECT_FALSE(trayIcon->icon().isNull())
        << "Tray icon should have an icon set on construction";
}

TEST_F(TrayIconTest, IconIsVisibleAfterShow) {
    trayIcon->show();
    QApplication::processEvents();

    EXPECT_TRUE(trayIcon->isVisible() || !QSystemTrayIcon::isSystemTrayAvailable())
        << "Tray icon should be visible if system tray is available";
}

// =============================================================================
// Basic Functionality Tests
// =============================================================================

TEST_F(TrayIconTest, UpdateIconSetsCorrectState) {
    trayIcon->updateIcon(true);
    QApplication::processEvents();
    EXPECT_FALSE(trayIcon->icon().isNull());

    trayIcon->updateIcon(false);
    QApplication::processEvents();
    EXPECT_FALSE(trayIcon->icon().isNull());
}

TEST_F(TrayIconTest, UpdateTooltipSetsText) {
    QString tooltipText = "Custom tooltip";
    trayIcon->updateTooltip(tooltipText);
    QApplication::processEvents();

    EXPECT_EQ(trayIcon->toolTip(), tooltipText);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
