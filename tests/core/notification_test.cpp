/**
 * @file notification_test.cpp
 * @brief Comprehensive tests for the NotificationDispatcher system
 *
 * Tests cover:
 * - Callback registration and unregistration
 * - Type filtering for callbacks
 * - Thread-safe dispatch
 * - Exception handling in callbacks
 * - Concurrent registration/dispatch
 */

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "core/notification_dispatcher.h"

using namespace yamy;
using namespace yamy::core;

// =============================================================================
// Test Fixture Setup
// =============================================================================

class NotificationDispatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear all callbacks before each test
        NotificationDispatcher::instance().clearCallbacks();
    }

    void TearDown() override {
        // Clear after test
        NotificationDispatcher::instance().clearCallbacks();
    }
};

// =============================================================================
// Basic Registration Tests
// =============================================================================

TEST_F(NotificationDispatcherTest, SingletonReturnsConsistentInstance) {
    NotificationDispatcher& instance1 = NotificationDispatcher::instance();
    NotificationDispatcher& instance2 = NotificationDispatcher::instance();

    EXPECT_EQ(&instance1, &instance2) << "Should return same singleton instance";
}

TEST_F(NotificationDispatcherTest, InitialCallbackCountIsZero) {
    EXPECT_EQ(NotificationDispatcher::instance().callbackCount(), 0u);
}

TEST_F(NotificationDispatcherTest, RegisterCallbackIncrementsCount) {
    auto handle = NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});

    EXPECT_EQ(NotificationDispatcher::instance().callbackCount(), 1u);
    EXPECT_GT(handle, 0u) << "Handle should be positive";
}

TEST_F(NotificationDispatcherTest, RegisterMultipleCallbacksIncrementsCount) {
    NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});
    NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});
    NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});

    EXPECT_EQ(NotificationDispatcher::instance().callbackCount(), 3u);
}

TEST_F(NotificationDispatcherTest, RegisterReturnsUniqueHandles) {
    auto handle1 = NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});
    auto handle2 = NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});
    auto handle3 = NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});

    EXPECT_NE(handle1, handle2);
    EXPECT_NE(handle2, handle3);
    EXPECT_NE(handle1, handle3);
}

// =============================================================================
// Unregistration Tests
// =============================================================================

TEST_F(NotificationDispatcherTest, UnregisterCallbackDecrementsCount) {
    auto handle = NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});

    EXPECT_EQ(NotificationDispatcher::instance().callbackCount(), 1u);

    bool result = NotificationDispatcher::instance().unregisterCallback(handle);

    EXPECT_TRUE(result);
    EXPECT_EQ(NotificationDispatcher::instance().callbackCount(), 0u);
}

TEST_F(NotificationDispatcherTest, UnregisterInvalidHandleReturnsFalse) {
    bool result = NotificationDispatcher::instance().unregisterCallback(9999);

    EXPECT_FALSE(result);
}

TEST_F(NotificationDispatcherTest, UnregisterSameHandleTwiceReturnsFalse) {
    auto handle = NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});

    EXPECT_TRUE(NotificationDispatcher::instance().unregisterCallback(handle));
    EXPECT_FALSE(NotificationDispatcher::instance().unregisterCallback(handle));
}

TEST_F(NotificationDispatcherTest, UnregisterMiddleCallback) {
    auto handle1 = NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});
    auto handle2 = NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});
    auto handle3 = NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});

    EXPECT_EQ(NotificationDispatcher::instance().callbackCount(), 3u);

    EXPECT_TRUE(NotificationDispatcher::instance().unregisterCallback(handle2));

    EXPECT_EQ(NotificationDispatcher::instance().callbackCount(), 2u);

    // Other handles should still be valid to unregister
    EXPECT_TRUE(NotificationDispatcher::instance().unregisterCallback(handle1));
    EXPECT_TRUE(NotificationDispatcher::instance().unregisterCallback(handle3));
}

// =============================================================================
// Dispatch Tests - All Types
// =============================================================================

TEST_F(NotificationDispatcherTest, DispatchInvokesCallback) {
    bool invoked = false;
    MessageType receivedType = MessageType::EngineStarting;
    std::string receivedData;

    NotificationDispatcher::instance().registerCallback(
        [&](MessageType type, const std::string& data) {
            invoked = true;
            receivedType = type;
            receivedData = data;
        });

    NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, "test_data");

    EXPECT_TRUE(invoked);
    EXPECT_EQ(receivedType, MessageType::EngineStarted);
    EXPECT_EQ(receivedData, "test_data");
}

TEST_F(NotificationDispatcherTest, DispatchInvokesAllCallbacks) {
    std::atomic<int> invokeCount{0};

    NotificationDispatcher::instance().registerCallback(
        [&](MessageType, const std::string&) { invokeCount++; });
    NotificationDispatcher::instance().registerCallback(
        [&](MessageType, const std::string&) { invokeCount++; });
    NotificationDispatcher::instance().registerCallback(
        [&](MessageType, const std::string&) { invokeCount++; });

    NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, "");

    EXPECT_EQ(invokeCount.load(), 3);
}

TEST_F(NotificationDispatcherTest, DispatchWithEmptyData) {
    std::string receivedData = "initial";

    NotificationDispatcher::instance().registerCallback(
        [&](MessageType, const std::string& data) { receivedData = data; });

    NotificationDispatcher::instance().dispatch(MessageType::EngineStarted);

    EXPECT_EQ(receivedData, "");
}

TEST_F(NotificationDispatcherTest, DispatchAllMessageTypes) {
    std::vector<MessageType> receivedTypes;

    NotificationDispatcher::instance().registerCallback(
        [&](MessageType type, const std::string&) { receivedTypes.push_back(type); });

    // Test all message types
    std::vector<MessageType> allTypes = {
        MessageType::EngineStarting,
        MessageType::EngineStarted,
        MessageType::EngineStopping,
        MessageType::EngineStopped,
        MessageType::EngineError,
        MessageType::ConfigLoading,
        MessageType::ConfigLoaded,
        MessageType::ConfigError,
        MessageType::ConfigValidating,
        MessageType::KeymapSwitched,
        MessageType::FocusChanged,
        MessageType::ModifierChanged,
        MessageType::LatencyReport,
        MessageType::CpuUsageReport
    };

    for (auto type : allTypes) {
        NotificationDispatcher::instance().dispatch(type, "");
    }

    EXPECT_EQ(receivedTypes.size(), allTypes.size());
    for (size_t i = 0; i < allTypes.size(); ++i) {
        EXPECT_EQ(receivedTypes[i], allTypes[i]);
    }
}

// =============================================================================
// Type Filtering Tests
// =============================================================================

TEST_F(NotificationDispatcherTest, FilteredCallbackReceivesOnlyMatchingTypes) {
    std::vector<MessageType> receivedTypes;

    std::unordered_set<MessageType> filter = {
        MessageType::EngineStarted,
        MessageType::EngineStopped
    };

    NotificationDispatcher::instance().registerCallback(
        filter,
        [&](MessageType type, const std::string&) { receivedTypes.push_back(type); });

    NotificationDispatcher::instance().dispatch(MessageType::EngineStarting, "");
    NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, "");
    NotificationDispatcher::instance().dispatch(MessageType::EngineStopping, "");
    NotificationDispatcher::instance().dispatch(MessageType::EngineStopped, "");
    NotificationDispatcher::instance().dispatch(MessageType::ConfigLoaded, "");

    EXPECT_EQ(receivedTypes.size(), 2u);
    EXPECT_EQ(receivedTypes[0], MessageType::EngineStarted);
    EXPECT_EQ(receivedTypes[1], MessageType::EngineStopped);
}

TEST_F(NotificationDispatcherTest, UnfilteredCallbackReceivesAllTypes) {
    std::vector<MessageType> receivedTypes;

    // Register with empty filter (all types)
    NotificationDispatcher::instance().registerCallback(
        [&](MessageType type, const std::string&) { receivedTypes.push_back(type); });

    NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, "");
    NotificationDispatcher::instance().dispatch(MessageType::ConfigLoaded, "");
    NotificationDispatcher::instance().dispatch(MessageType::KeymapSwitched, "");

    EXPECT_EQ(receivedTypes.size(), 3u);
}

TEST_F(NotificationDispatcherTest, MixedFilteredAndUnfilteredCallbacks) {
    std::atomic<int> unfilteredCount{0};
    std::atomic<int> filteredCount{0};

    // Unfiltered callback
    NotificationDispatcher::instance().registerCallback(
        [&](MessageType, const std::string&) { unfilteredCount++; });

    // Filtered callback
    NotificationDispatcher::instance().registerCallback(
        {MessageType::EngineError, MessageType::ConfigError},
        [&](MessageType, const std::string&) { filteredCount++; });

    NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, "");
    NotificationDispatcher::instance().dispatch(MessageType::EngineError, "");
    NotificationDispatcher::instance().dispatch(MessageType::ConfigLoaded, "");
    NotificationDispatcher::instance().dispatch(MessageType::ConfigError, "");

    EXPECT_EQ(unfilteredCount.load(), 4);
    EXPECT_EQ(filteredCount.load(), 2);
}

TEST_F(NotificationDispatcherTest, SingleTypeFilter) {
    int invokeCount = 0;

    NotificationDispatcher::instance().registerCallback(
        {MessageType::EngineError},
        [&](MessageType, const std::string&) { invokeCount++; });

    NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, "");
    NotificationDispatcher::instance().dispatch(MessageType::EngineError, "");
    NotificationDispatcher::instance().dispatch(MessageType::EngineError, "");

    EXPECT_EQ(invokeCount, 2);
}

// =============================================================================
// Exception Handling Tests
// =============================================================================

TEST_F(NotificationDispatcherTest, ExceptionInCallbackDoesNotCrash) {
    NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) { throw std::runtime_error("Test exception"); });

    // Should not throw
    EXPECT_NO_THROW(NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, ""));
}

TEST_F(NotificationDispatcherTest, ExceptionInCallbackDoesNotPreventOtherCallbacks) {
    std::atomic<int> invokeCount{0};

    // First callback - will throw
    NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) { throw std::runtime_error("Test exception"); });

    // Second callback - should still be invoked
    NotificationDispatcher::instance().registerCallback(
        [&](MessageType, const std::string&) { invokeCount++; });

    // Third callback - should still be invoked
    NotificationDispatcher::instance().registerCallback(
        [&](MessageType, const std::string&) { invokeCount++; });

    NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, "");

    EXPECT_EQ(invokeCount.load(), 2);
}

TEST_F(NotificationDispatcherTest, UnknownExceptionInCallbackHandled) {
    NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) { throw 42; });  // Non-std::exception

    EXPECT_NO_THROW(NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, ""));
}

// =============================================================================
// Clear Tests
// =============================================================================

TEST_F(NotificationDispatcherTest, ClearRemovesAllCallbacks) {
    NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});
    NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});

    EXPECT_EQ(NotificationDispatcher::instance().callbackCount(), 2u);

    NotificationDispatcher::instance().clearCallbacks();

    EXPECT_EQ(NotificationDispatcher::instance().callbackCount(), 0u);
}

TEST_F(NotificationDispatcherTest, ClearCallbacksDispatchDoesNothing) {
    int invokeCount = 0;

    NotificationDispatcher::instance().registerCallback(
        [&](MessageType, const std::string&) { invokeCount++; });

    NotificationDispatcher::instance().clearCallbacks();
    NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, "");

    EXPECT_EQ(invokeCount, 0);
}

// =============================================================================
// Thread Safety Tests
// =============================================================================

TEST_F(NotificationDispatcherTest, ConcurrentRegistrationIsThreadSafe) {
    constexpr int numThreads = 4;
    constexpr int registrationsPerThread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([]() {
            for (int i = 0; i < registrationsPerThread; ++i) {
                NotificationDispatcher::instance().registerCallback(
                    [](MessageType, const std::string&) {});
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(NotificationDispatcher::instance().callbackCount(),
              static_cast<size_t>(numThreads * registrationsPerThread));
}

TEST_F(NotificationDispatcherTest, ConcurrentDispatchIsThreadSafe) {
    std::atomic<int> invokeCount{0};

    NotificationDispatcher::instance().registerCallback(
        [&](MessageType, const std::string&) { invokeCount++; });

    constexpr int numThreads = 4;
    constexpr int dispatchesPerThread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([]() {
            for (int i = 0; i < dispatchesPerThread; ++i) {
                NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, "");
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(invokeCount.load(), numThreads * dispatchesPerThread);
}

TEST_F(NotificationDispatcherTest, ConcurrentRegistrationAndDispatch) {
    std::atomic<int> invokeCount{0};
    std::atomic<bool> running{true};

    // Start with one callback
    NotificationDispatcher::instance().registerCallback(
        [&](MessageType, const std::string&) { invokeCount++; });

    // Dispatcher thread
    std::thread dispatcher([&]() {
        for (int i = 0; i < 100 && running; ++i) {
            NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, "");
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });

    // Registration thread
    std::thread registrar([&]() {
        for (int i = 0; i < 50 && running; ++i) {
            NotificationDispatcher::instance().registerCallback(
                [&](MessageType, const std::string&) { invokeCount++; });
            std::this_thread::sleep_for(std::chrono::microseconds(20));
        }
    });

    dispatcher.join();
    running = false;
    registrar.join();

    // Should not have crashed and should have received some dispatches
    EXPECT_GT(invokeCount.load(), 0);
}

TEST_F(NotificationDispatcherTest, ConcurrentUnregistration) {
    std::vector<CallbackHandle> handles;

    // Register many callbacks
    for (int i = 0; i < 100; ++i) {
        handles.push_back(NotificationDispatcher::instance().registerCallback(
            [](MessageType, const std::string&) {}));
    }

    // Unregister from multiple threads
    std::vector<std::thread> threads;
    size_t handlesPerThread = handles.size() / 4;

    for (int t = 0; t < 4; ++t) {
        size_t start = t * handlesPerThread;
        size_t end = (t == 3) ? handles.size() : start + handlesPerThread;

        threads.emplace_back([&handles, start, end]() {
            for (size_t i = start; i < end; ++i) {
                NotificationDispatcher::instance().unregisterCallback(handles[i]);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(NotificationDispatcher::instance().callbackCount(), 0u);
}

// =============================================================================
// Callback During Dispatch Tests
// =============================================================================

TEST_F(NotificationDispatcherTest, DispatchSnapshotsCallbacksToPreventDeadlock) {
    // This tests that callbacks can safely modify the callback list during dispatch
    // because dispatch takes a snapshot of callbacks before invoking

    int invokeCount = 0;
    CallbackHandle selfHandle;

    // This callback will try to unregister itself during dispatch
    selfHandle = NotificationDispatcher::instance().registerCallback(
        [&](MessageType, const std::string&) {
            invokeCount++;
            // This should not deadlock because dispatch takes a snapshot
            // Unregistration will happen after the snapshot, so it's safe
            NotificationDispatcher::instance().unregisterCallback(selfHandle);
        });

    NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, "");

    EXPECT_EQ(invokeCount, 1);
    // Callback should have been unregistered
    EXPECT_EQ(NotificationDispatcher::instance().callbackCount(), 0u);
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(NotificationDispatcherTest, DispatchPerformance) {
    NotificationDispatcher::instance().registerCallback(
        [](MessageType, const std::string&) {});

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        NotificationDispatcher::instance().dispatch(MessageType::EngineStarted, "test_data");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_LT(duration.count(), 1000) << "10000 dispatches should complete in <1s";
}

TEST_F(NotificationDispatcherTest, RegistrationPerformance) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000; ++i) {
        NotificationDispatcher::instance().registerCallback(
            [](MessageType, const std::string&) {});
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_LT(duration.count(), 1000) << "1000 registrations should complete in <1s";
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
