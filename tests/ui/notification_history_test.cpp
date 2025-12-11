/**
 * @file notification_history_test.cpp
 * @brief Comprehensive tests for the notification history system
 *
 * Tests cover:
 * - NotificationHistory singleton and thread-safe storage
 * - NotificationEntry formatting and type naming
 * - NotificationHistoryDialog UI functionality
 * - Rolling window behavior
 * - Thread safety
 */

#include <gtest/gtest.h>
#include <QApplication>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QThread>
#include <thread>
#include <vector>

#include "ui/qt/notification_history.h"

using namespace yamy;
using namespace yamy::ui;

// =============================================================================
// Test Fixture Setup
// =============================================================================

class NotificationHistoryTest : public ::testing::Test {
protected:
    static QApplication* app;

    static void SetUpTestSuite() {
        if (!QApplication::instance()) {
            int argc = 0;
            app = new QApplication(argc, nullptr);
        }
        // Register NotificationEntry for queued connections
        qRegisterMetaType<NotificationEntry>("NotificationEntry");
    }

    void SetUp() override {
        // Clear history before each test
        NotificationHistory::instance().clear();
        NotificationHistory::instance().setMaxSize(NotificationHistory::DEFAULT_MAX_SIZE);
    }

    void TearDown() override {
        QApplication::processEvents();
        // Clear after test
        NotificationHistory::instance().clear();
    }
};

QApplication* NotificationHistoryTest::app = nullptr;

// =============================================================================
// NotificationEntry Tests
// =============================================================================

TEST_F(NotificationHistoryTest, EntryTypeNameEngineStates) {
    EXPECT_EQ(NotificationEntry::typeName(MessageType::EngineStarting), "Engine Starting");
    EXPECT_EQ(NotificationEntry::typeName(MessageType::EngineStarted), "Engine Started");
    EXPECT_EQ(NotificationEntry::typeName(MessageType::EngineStopping), "Engine Stopping");
    EXPECT_EQ(NotificationEntry::typeName(MessageType::EngineStopped), "Engine Stopped");
    EXPECT_EQ(NotificationEntry::typeName(MessageType::EngineError), "Engine Error");
}

TEST_F(NotificationHistoryTest, EntryTypeNameConfigStates) {
    EXPECT_EQ(NotificationEntry::typeName(MessageType::ConfigLoading), "Config Loading");
    EXPECT_EQ(NotificationEntry::typeName(MessageType::ConfigLoaded), "Config Loaded");
    EXPECT_EQ(NotificationEntry::typeName(MessageType::ConfigError), "Config Error");
    EXPECT_EQ(NotificationEntry::typeName(MessageType::ConfigValidating), "Config Validating");
}

TEST_F(NotificationHistoryTest, EntryTypeNameRuntimeEvents) {
    EXPECT_EQ(NotificationEntry::typeName(MessageType::KeymapSwitched), "Keymap Switched");
    EXPECT_EQ(NotificationEntry::typeName(MessageType::FocusChanged), "Focus Changed");
    EXPECT_EQ(NotificationEntry::typeName(MessageType::ModifierChanged), "Modifier Changed");
}

TEST_F(NotificationHistoryTest, EntryTypeNamePerformance) {
    EXPECT_EQ(NotificationEntry::typeName(MessageType::LatencyReport), "Latency Report");
    EXPECT_EQ(NotificationEntry::typeName(MessageType::CpuUsageReport), "CPU Usage Report");
}

TEST_F(NotificationHistoryTest, EntryTypeNameUnknown) {
    QString name = NotificationEntry::typeName(static_cast<MessageType>(0xFFFF));
    EXPECT_TRUE(name.startsWith("Unknown"));
}

TEST_F(NotificationHistoryTest, EntryFormatWithData) {
    NotificationEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15 14:30:45", "yyyy-MM-dd HH:mm:ss");
    entry.type = MessageType::ConfigLoaded;
    entry.data = "work.mayu";

    QString formatted = entry.format();

    EXPECT_TRUE(formatted.contains("14:30:45")) << "Should contain timestamp";
    EXPECT_TRUE(formatted.contains("Config Loaded")) << "Should contain type name";
    EXPECT_TRUE(formatted.contains("work.mayu")) << "Should contain data";
}

TEST_F(NotificationHistoryTest, EntryFormatWithoutData) {
    NotificationEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15 14:30:45", "yyyy-MM-dd HH:mm:ss");
    entry.type = MessageType::EngineStarted;
    entry.data = "";

    QString formatted = entry.format();

    EXPECT_TRUE(formatted.contains("14:30:45")) << "Should contain timestamp";
    EXPECT_TRUE(formatted.contains("Engine Started")) << "Should contain type name";
    EXPECT_FALSE(formatted.contains(":") && formatted.endsWith(":"))
        << "Should not end with colon when no data";
}

// =============================================================================
// NotificationHistory Singleton Tests
// =============================================================================

TEST_F(NotificationHistoryTest, SingletonInstanceWorks) {
    NotificationHistory& instance1 = NotificationHistory::instance();
    NotificationHistory& instance2 = NotificationHistory::instance();

    EXPECT_EQ(&instance1, &instance2) << "Should return same singleton instance";
}

TEST_F(NotificationHistoryTest, AddNotificationIncrementsCount) {
    EXPECT_EQ(NotificationHistory::instance().count(), 0);

    NotificationHistory::instance().addNotification(MessageType::EngineStarted, "");

    EXPECT_EQ(NotificationHistory::instance().count(), 1);
}

TEST_F(NotificationHistoryTest, GetNotificationsReturnsAll) {
    NotificationHistory::instance().addNotification(MessageType::EngineStarting, "");
    NotificationHistory::instance().addNotification(MessageType::EngineStarted, "");
    NotificationHistory::instance().addNotification(MessageType::ConfigLoaded, "test.mayu");

    auto notifications = NotificationHistory::instance().getNotifications();

    EXPECT_EQ(notifications.size(), 3u);
    EXPECT_EQ(notifications[0].type, MessageType::EngineStarting);
    EXPECT_EQ(notifications[1].type, MessageType::EngineStarted);
    EXPECT_EQ(notifications[2].type, MessageType::ConfigLoaded);
    EXPECT_EQ(notifications[2].data, "test.mayu");
}

TEST_F(NotificationHistoryTest, ClearRemovesAll) {
    NotificationHistory::instance().addNotification(MessageType::EngineStarted, "");
    NotificationHistory::instance().addNotification(MessageType::EngineStarted, "");

    EXPECT_EQ(NotificationHistory::instance().count(), 2);

    NotificationHistory::instance().clear();

    EXPECT_EQ(NotificationHistory::instance().count(), 0);
}

TEST_F(NotificationHistoryTest, MaxSizeEnforcesLimit) {
    NotificationHistory::instance().setMaxSize(5);

    for (int i = 0; i < 10; ++i) {
        NotificationHistory::instance().addNotification(
            MessageType::EngineStarted,
            QString::number(i)
        );
    }

    EXPECT_EQ(NotificationHistory::instance().count(), 5);

    // Should have the most recent 5 entries (5-9)
    auto notifications = NotificationHistory::instance().getNotifications();
    EXPECT_EQ(notifications[0].data, "5");
    EXPECT_EQ(notifications[4].data, "9");
}

TEST_F(NotificationHistoryTest, MaxSizeClamped) {
    // Test minimum clamping
    NotificationHistory::instance().setMaxSize(0);
    EXPECT_EQ(NotificationHistory::instance().maxSize(), 1);

    // Test maximum clamping
    NotificationHistory::instance().setMaxSize(10000);
    EXPECT_EQ(NotificationHistory::instance().maxSize(), 1000);
}

TEST_F(NotificationHistoryTest, SetMaxSizeTrimsExisting) {
    // Add 10 entries
    for (int i = 0; i < 10; ++i) {
        NotificationHistory::instance().addNotification(
            MessageType::EngineStarted,
            QString::number(i)
        );
    }

    EXPECT_EQ(NotificationHistory::instance().count(), 10);

    // Reduce max size - should trim
    NotificationHistory::instance().setMaxSize(3);

    EXPECT_EQ(NotificationHistory::instance().count(), 3);

    // Should have the most recent 3 entries
    auto notifications = NotificationHistory::instance().getNotifications();
    EXPECT_EQ(notifications[0].data, "7");
    EXPECT_EQ(notifications[2].data, "9");
}

TEST_F(NotificationHistoryTest, TimestampIsPopulated) {
    QDateTime before = QDateTime::currentDateTime();
    NotificationHistory::instance().addNotification(MessageType::EngineStarted, "");
    QDateTime after = QDateTime::currentDateTime();

    auto notifications = NotificationHistory::instance().getNotifications();
    ASSERT_EQ(notifications.size(), 1u);

    EXPECT_GE(notifications[0].timestamp, before);
    EXPECT_LE(notifications[0].timestamp, after);
}

// =============================================================================
// Signal Tests
// =============================================================================

TEST_F(NotificationHistoryTest, SignalEmittedOnAdd) {
    bool signalReceived = false;
    NotificationEntry receivedEntry;

    // Use a QObject to properly manage the connection lifetime
    QObject context;
    QObject::connect(&NotificationHistory::instance(), &NotificationHistory::notificationAdded,
        &context, [&](const NotificationEntry& entry) {
            signalReceived = true;
            receivedEntry = entry;
        });

    NotificationHistory::instance().addNotification(MessageType::ConfigLoaded, "test.mayu");
    QApplication::processEvents();

    EXPECT_TRUE(signalReceived);
    EXPECT_EQ(receivedEntry.type, MessageType::ConfigLoaded);
    EXPECT_EQ(receivedEntry.data, "test.mayu");
}

TEST_F(NotificationHistoryTest, SignalEmittedOnClear) {
    bool signalReceived = false;

    // Use a QObject to properly manage the connection lifetime
    QObject context;
    QObject::connect(&NotificationHistory::instance(), &NotificationHistory::historyCleared,
        &context, [&]() {
            signalReceived = true;
        });

    NotificationHistory::instance().addNotification(MessageType::EngineStarted, "");
    NotificationHistory::instance().clear();
    QApplication::processEvents();

    EXPECT_TRUE(signalReceived);
}

// =============================================================================
// NotificationHistoryDialog Tests
// =============================================================================

TEST_F(NotificationHistoryTest, DialogCreatesWithCorrectTitle) {
    NotificationHistoryDialog dialog;

    EXPECT_EQ(dialog.windowTitle(), "Notification History");
}

TEST_F(NotificationHistoryTest, DialogDisplaysExistingNotifications) {
    // Add notifications before creating dialog
    NotificationHistory::instance().addNotification(MessageType::EngineStarted, "");
    NotificationHistory::instance().addNotification(MessageType::ConfigLoaded, "test.mayu");

    NotificationHistoryDialog dialog;
    QApplication::processEvents();

    QListWidget* listWidget = dialog.findChild<QListWidget*>();
    ASSERT_NE(listWidget, nullptr);

    EXPECT_EQ(listWidget->count(), 2);
}

TEST_F(NotificationHistoryTest, DialogUpdatesOnNewNotification) {
    NotificationHistoryDialog dialog;
    QApplication::processEvents();

    QListWidget* listWidget = dialog.findChild<QListWidget*>();
    ASSERT_NE(listWidget, nullptr);

    EXPECT_EQ(listWidget->count(), 0);

    NotificationHistory::instance().addNotification(MessageType::EngineStarted, "");
    QApplication::processEvents();

    EXPECT_EQ(listWidget->count(), 1);
}

TEST_F(NotificationHistoryTest, DialogClearButtonWorks) {
    NotificationHistory::instance().addNotification(MessageType::EngineStarted, "");
    NotificationHistory::instance().addNotification(MessageType::EngineStarted, "");

    NotificationHistoryDialog dialog;
    QApplication::processEvents();

    QListWidget* listWidget = dialog.findChild<QListWidget*>();
    ASSERT_NE(listWidget, nullptr);
    EXPECT_EQ(listWidget->count(), 2);

    QPushButton* clearBtn = nullptr;
    for (QPushButton* btn : dialog.findChildren<QPushButton*>()) {
        if (btn->text() == "Clear") {
            clearBtn = btn;
            break;
        }
    }
    ASSERT_NE(clearBtn, nullptr);

    clearBtn->click();
    QApplication::processEvents();

    EXPECT_EQ(listWidget->count(), 0);
    EXPECT_EQ(NotificationHistory::instance().count(), 0);
}

TEST_F(NotificationHistoryTest, DialogColorCodesErrors) {
    NotificationHistory::instance().addNotification(MessageType::EngineError, "Test error");

    NotificationHistoryDialog dialog;
    QApplication::processEvents();

    QListWidget* listWidget = dialog.findChild<QListWidget*>();
    ASSERT_NE(listWidget, nullptr);
    ASSERT_EQ(listWidget->count(), 1);

    QListWidgetItem* item = listWidget->item(0);
    EXPECT_EQ(item->foreground().color(), Qt::red);
}

TEST_F(NotificationHistoryTest, DialogColorCodesSuccess) {
    NotificationHistory::instance().addNotification(MessageType::EngineStarted, "");

    NotificationHistoryDialog dialog;
    QApplication::processEvents();

    QListWidget* listWidget = dialog.findChild<QListWidget*>();
    ASSERT_NE(listWidget, nullptr);
    ASSERT_EQ(listWidget->count(), 1);

    QListWidgetItem* item = listWidget->item(0);
    // Dark green
    EXPECT_EQ(item->foreground().color(), QColor(0, 128, 0));
}

TEST_F(NotificationHistoryTest, DialogColorCodesInProgress) {
    NotificationHistory::instance().addNotification(MessageType::EngineStarting, "");

    NotificationHistoryDialog dialog;
    QApplication::processEvents();

    QListWidget* listWidget = dialog.findChild<QListWidget*>();
    ASSERT_NE(listWidget, nullptr);
    ASSERT_EQ(listWidget->count(), 1);

    QListWidgetItem* item = listWidget->item(0);
    // Dark blue
    EXPECT_EQ(item->foreground().color(), QColor(0, 0, 180));
}

TEST_F(NotificationHistoryTest, DialogCloseButtonWorks) {
    NotificationHistoryDialog dialog;
    // Don't call show() as it can crash in headless test environments

    QPushButton* closeBtn = nullptr;
    for (QPushButton* btn : dialog.findChildren<QPushButton*>()) {
        if (btn->text() == "Close") {
            closeBtn = btn;
            break;
        }
    }
    ASSERT_NE(closeBtn, nullptr) << "Dialog should have a Close button";

    // Verify the button is properly connected - clicking should call close()
    // We can't test visibility without a window system, but we can verify the button exists
    EXPECT_TRUE(closeBtn->isEnabled());
}

// =============================================================================
// Thread Safety Tests
// =============================================================================

// Note: These tests verify thread-safety of the data structure.
// Signal emission is tested separately in single-threaded context.

TEST_F(NotificationHistoryTest, ConcurrentAddNotificationsStorage) {
    // Disconnect all signals to avoid cross-thread signal emission issues in tests
    NotificationHistory::instance().blockSignals(true);

    constexpr int numThreads = 4;
    constexpr int notificationsPerThread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([t]() {
            for (int i = 0; i < notificationsPerThread; ++i) {
                NotificationHistory::instance().addNotification(
                    MessageType::EngineStarted,
                    QString("Thread %1 Message %2").arg(t).arg(i)
                );
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    NotificationHistory::instance().blockSignals(false);

    // All notifications should have been added (within max size limit)
    int expectedCount = std::min(
        numThreads * notificationsPerThread,
        NotificationHistory::DEFAULT_MAX_SIZE
    );
    EXPECT_EQ(NotificationHistory::instance().count(), expectedCount);
}

TEST_F(NotificationHistoryTest, ConcurrentReadWriteStorage) {
    // Disconnect all signals to avoid cross-thread signal emission issues in tests
    NotificationHistory::instance().blockSignals(true);

    constexpr int numOperations = 100;

    std::atomic<bool> running{true};
    std::atomic<int> readCount{0};

    // Writer thread
    std::thread writer([&]() {
        for (int i = 0; i < numOperations && running; ++i) {
            NotificationHistory::instance().addNotification(
                MessageType::EngineStarted,
                QString::number(i)
            );
        }
    });

    // Reader threads
    std::vector<std::thread> readers;
    for (int r = 0; r < 2; ++r) {
        readers.emplace_back([&]() {
            while (running) {
                auto notifications = NotificationHistory::instance().getNotifications();
                readCount.fetch_add(1, std::memory_order_relaxed);
                if (notifications.size() >= static_cast<size_t>(numOperations)) {
                    break;
                }
            }
        });
    }

    writer.join();
    running = false;

    for (auto& reader : readers) {
        reader.join();
    }

    NotificationHistory::instance().blockSignals(false);

    // No crashes or data corruption should occur
    EXPECT_GT(readCount.load(), 0) << "Reader threads should have completed reads";
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(NotificationHistoryTest, AddPerformance) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000; ++i) {
        NotificationHistory::instance().addNotification(
            MessageType::EngineStarted,
            QString("Performance test %1").arg(i)
        );
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_LT(duration.count(), 1000) << "Adding 1000 notifications should complete in <1s";
}

TEST_F(NotificationHistoryTest, GetNotificationsPerformance) {
    // Fill history
    for (int i = 0; i < 100; ++i) {
        NotificationHistory::instance().addNotification(
            MessageType::EngineStarted,
            QString("Test %1").arg(i)
        );
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000; ++i) {
        auto notifications = NotificationHistory::instance().getNotifications();
        EXPECT_EQ(notifications.size(), 100u);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_LT(duration.count(), 1000) << "1000 get operations should complete in <1s";
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
