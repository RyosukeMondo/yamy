/**
 * @file ipc_channel_qt_test.cpp
 * @brief Comprehensive tests for IPCChannelQt message serialization and communication
 *
 * Tests cover:
 * - Send/receive round-trip for all message types
 * - Connection/disconnection handling
 * - Partial message buffering (message framing)
 * - Large messages, connection refused, timeout
 * - Server and client mode operations
 */

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTest>
#include <QString>
#include <QSignalSpy>
#include <memory>

#include "core/platform/linux/ipc_channel_qt.h"
#include "core/ipc_messages.h"

using namespace yamy::platform;
using namespace yamy::ipc;

// =============================================================================
// Test Fixture Setup
// =============================================================================

class IPCChannelQtTest : public ::testing::Test {
protected:
    static QCoreApplication* app;

    static void SetUpTestSuite() {
        if (!QCoreApplication::instance()) {
            int argc = 0;
            app = new QCoreApplication(argc, nullptr);
        }
    }

    void SetUp() override {
        // Create unique server name for each test to avoid conflicts
        static int testCounter = 0;
        serverName = "test-server-" + std::to_string(testCounter++);

        server = new IPCChannelQt(serverName);
        client = new IPCChannelQt("test-client");

        server->listen();
        client->connect(serverName);

        // Wait for connection to establish
        QTest::qWait(100);
    }

    void TearDown() override {
        delete client;
        delete server;
        client = nullptr;
        server = nullptr;
        QCoreApplication::processEvents();
    }

    IPCChannelQt* server = nullptr;
    IPCChannelQt* client = nullptr;
    std::string serverName;
};

QCoreApplication* IPCChannelQtTest::app = nullptr;

// =============================================================================
// Connection Tests
// =============================================================================

TEST_F(IPCChannelQtTest, ClientConnectsToServer) {
    ASSERT_TRUE(client->isConnected()) << "Client should connect to server";
}

TEST_F(IPCChannelQtTest, ServerIsConnectedAfterClientConnects) {
    ASSERT_TRUE(server->isConnected()) << "Server should report connected after client connects";
}

TEST_F(IPCChannelQtTest, ConnectionRefusedHandling) {
    IPCChannelQt* failClient = new IPCChannelQt("fail-client");
    failClient->connect("nonexistent-server-xyz123");

    QTest::qWait(100);
    EXPECT_FALSE(failClient->isConnected()) << "Client should not connect to non-existent server";

    delete failClient;
}

TEST_F(IPCChannelQtTest, DisconnectClearsConnectedState) {
    ASSERT_TRUE(client->isConnected());

    client->disconnect();
    QTest::qWait(50);

    EXPECT_FALSE(client->isConnected()) << "Client should not be connected after disconnect";
}

// =============================================================================
// Message Serialization Tests
// =============================================================================

TEST_F(IPCChannelQtTest, MessageRoundTrip_InvestigateWindowRequest) {
    InvestigateWindowRequest request;
    request.hwnd = reinterpret_cast<void*>(0x12345678);

    Message msg;
    msg.type = CmdInvestigateWindow;
    msg.data = &request;
    msg.size = sizeof(request);

    bool received = false;
    void* receivedHwnd = nullptr;

    QObject::connect(server, &IPCChannelQt::messageReceived,
                     [&](const Message& receivedMsg) {
        EXPECT_EQ(CmdInvestigateWindow, receivedMsg.type);
        EXPECT_EQ(sizeof(request), receivedMsg.size);

        auto* req = static_cast<const InvestigateWindowRequest*>(receivedMsg.data);
        receivedHwnd = req->hwnd;
        received = true;
    });

    client->send(msg);
    QTest::qWait(50);

    EXPECT_TRUE(received) << "Server should receive message";
    EXPECT_EQ(request.hwnd, receivedHwnd) << "Received hwnd should match sent hwnd";
}

TEST_F(IPCChannelQtTest, MessageRoundTrip_InvestigateWindowResponse) {
    InvestigateWindowResponse response;
    strncpy(response.keymapName, "TestKeymap", sizeof(response.keymapName) - 1);
    strncpy(response.matchedClassRegex, ".*Firefox.*", sizeof(response.matchedClassRegex) - 1);
    strncpy(response.matchedTitleRegex, ".*Mozilla.*", sizeof(response.matchedTitleRegex) - 1);
    strncpy(response.activeModifiers, "Ctrl+Shift", sizeof(response.activeModifiers) - 1);
    response.isDefault = false;

    Message msg;
    msg.type = RspInvestigateWindow;
    msg.data = &response;
    msg.size = sizeof(response);

    bool received = false;
    std::string receivedKeymap;
    std::string receivedClass;

    QObject::connect(client, &IPCChannelQt::messageReceived,
                     [&](const Message& receivedMsg) {
        EXPECT_EQ(RspInvestigateWindow, receivedMsg.type);
        EXPECT_EQ(sizeof(response), receivedMsg.size);

        auto* rsp = static_cast<const InvestigateWindowResponse*>(receivedMsg.data);
        receivedKeymap = rsp->keymapName;
        receivedClass = rsp->matchedClassRegex;
        EXPECT_FALSE(rsp->isDefault);
        received = true;
    });

    server->send(msg);
    QTest::qWait(50);

    EXPECT_TRUE(received) << "Client should receive response";
    EXPECT_EQ("TestKeymap", receivedKeymap);
    EXPECT_EQ(".*Firefox.*", receivedClass);
}

TEST_F(IPCChannelQtTest, MessageRoundTrip_KeyEventNotification) {
    KeyEventNotification notification;
    strncpy(notification.keyEvent, "[12:34:56.789] Ctrl-X ↓", sizeof(notification.keyEvent) - 1);

    Message msg;
    msg.type = NtfKeyEvent;
    msg.data = &notification;
    msg.size = sizeof(notification);

    bool received = false;
    std::string receivedEvent;

    QObject::connect(client, &IPCChannelQt::messageReceived,
                     [&](const Message& receivedMsg) {
        EXPECT_EQ(NtfKeyEvent, receivedMsg.type);
        auto* ntf = static_cast<const KeyEventNotification*>(receivedMsg.data);
        receivedEvent = ntf->keyEvent;
        received = true;
    });

    server->send(msg);
    QTest::qWait(50);

    EXPECT_TRUE(received);
    EXPECT_EQ("[12:34:56.789] Ctrl-X ↓", receivedEvent);
}

TEST_F(IPCChannelQtTest, MessageRoundTrip_EnableInvestigateMode) {
    Message msg;
    msg.type = CmdEnableInvestigateMode;
    msg.data = nullptr;
    msg.size = 0;

    bool received = false;
    MessageType receivedType = static_cast<MessageType>(0);

    QObject::connect(server, &IPCChannelQt::messageReceived,
                     [&](const Message& receivedMsg) {
        receivedType = receivedMsg.type;
        EXPECT_EQ(0u, receivedMsg.size);
        received = true;
    });

    client->send(msg);
    QTest::qWait(50);

    EXPECT_TRUE(received);
    EXPECT_EQ(CmdEnableInvestigateMode, receivedType);
}

TEST_F(IPCChannelQtTest, MessageRoundTrip_DisableInvestigateMode) {
    Message msg;
    msg.type = CmdDisableInvestigateMode;
    msg.data = nullptr;
    msg.size = 0;

    bool received = false;
    MessageType receivedType = static_cast<MessageType>(0);

    QObject::connect(server, &IPCChannelQt::messageReceived,
                     [&](const Message& receivedMsg) {
        receivedType = receivedMsg.type;
        received = true;
    });

    client->send(msg);
    QTest::qWait(50);

    EXPECT_TRUE(received);
    EXPECT_EQ(CmdDisableInvestigateMode, receivedType);
}

// =============================================================================
// Large Message Tests
// =============================================================================

TEST_F(IPCChannelQtTest, LargeMessageHandling) {
    // Test message larger than typical socket buffer (64KB)
    const size_t largeSize = 65536;
    std::vector<char> largeData(largeSize, 'A');

    // Fill with pattern to verify integrity
    for (size_t i = 0; i < largeSize; i++) {
        largeData[i] = static_cast<char>('A' + (i % 26));
    }

    Message msg;
    msg.type = NtfKeyEvent;
    msg.data = largeData.data();
    msg.size = largeSize;

    bool received = false;
    size_t receivedSize = 0;
    char firstChar = 0, lastChar = 0;

    QObject::connect(server, &IPCChannelQt::messageReceived,
                     [&](const Message& receivedMsg) {
        receivedSize = receivedMsg.size;
        if (receivedSize > 0) {
            const char* data = static_cast<const char*>(receivedMsg.data);
            firstChar = data[0];
            lastChar = data[receivedSize - 1];
        }
        received = true;
    });

    client->send(msg);
    QTest::qWait(200);  // Larger message may take more time

    EXPECT_TRUE(received) << "Large message should be received";
    EXPECT_EQ(largeSize, receivedSize) << "Received size should match sent size";
    EXPECT_EQ('A', firstChar) << "First byte should match";
    EXPECT_EQ(static_cast<char>('A' + ((largeSize - 1) % 26)), lastChar) << "Last byte should match";
}

// =============================================================================
// Multiple Message Tests
// =============================================================================

TEST_F(IPCChannelQtTest, MultipleMessagesInSequence) {
    int messageCount = 0;
    std::vector<MessageType> receivedTypes;

    QObject::connect(server, &IPCChannelQt::messageReceived,
                     [&](const Message& receivedMsg) {
        messageCount++;
        receivedTypes.push_back(receivedMsg.type);
    });

    // Send multiple messages rapidly
    for (int i = 0; i < 10; i++) {
        Message msg;
        msg.type = static_cast<MessageType>(CmdInvestigateWindow + i);
        msg.data = nullptr;
        msg.size = 0;
        client->send(msg);
    }

    QTest::qWait(100);

    EXPECT_EQ(10, messageCount) << "Should receive all 10 messages";
    EXPECT_EQ(10u, receivedTypes.size());

    // Verify order is preserved
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(static_cast<MessageType>(CmdInvestigateWindow + i), receivedTypes[i])
            << "Message order should be preserved";
    }
}

// =============================================================================
// Bidirectional Communication Tests
// =============================================================================

TEST_F(IPCChannelQtTest, BidirectionalCommunication) {
    int clientReceived = 0;
    int serverReceived = 0;

    QObject::connect(client, &IPCChannelQt::messageReceived,
                     [&](const Message&) { clientReceived++; });

    QObject::connect(server, &IPCChannelQt::messageReceived,
                     [&](const Message&) { serverReceived++; });

    // Client sends to server
    Message msg1;
    msg1.type = CmdInvestigateWindow;
    msg1.data = nullptr;
    msg1.size = 0;
    client->send(msg1);

    // Server sends to client
    Message msg2;
    msg2.type = RspInvestigateWindow;
    msg2.data = nullptr;
    msg2.size = 0;
    server->send(msg2);

    QTest::qWait(100);

    EXPECT_EQ(1, serverReceived) << "Server should receive message from client";
    EXPECT_EQ(1, clientReceived) << "Client should receive message from server";
}

// =============================================================================
// Edge Case Tests
// =============================================================================

TEST_F(IPCChannelQtTest, SendWhileDisconnectedDoesNotCrash) {
    IPCChannelQt* disconnectedClient = new IPCChannelQt("disconnected");

    Message msg;
    msg.type = CmdInvestigateWindow;
    msg.data = nullptr;
    msg.size = 0;

    // Should not crash when sending while disconnected
    EXPECT_NO_THROW(disconnectedClient->send(msg));

    delete disconnectedClient;
}

TEST_F(IPCChannelQtTest, EmptyMessageHandling) {
    Message msg;
    msg.type = CmdInvestigateWindow;
    msg.data = nullptr;
    msg.size = 0;

    bool received = false;
    size_t receivedSize = 999;

    QObject::connect(server, &IPCChannelQt::messageReceived,
                     [&](const Message& receivedMsg) {
        receivedSize = receivedMsg.size;
        received = true;
    });

    client->send(msg);
    QTest::qWait(50);

    EXPECT_TRUE(received);
    EXPECT_EQ(0u, receivedSize) << "Empty message should have zero size";
}

TEST_F(IPCChannelQtTest, UnicodeStringHandling) {
    KeyEventNotification notification;
    // Japanese characters: "日本語キー"
    strncpy(notification.keyEvent, u8"[12:34:56.789] 日本語キー ↓", sizeof(notification.keyEvent) - 1);

    Message msg;
    msg.type = NtfKeyEvent;
    msg.data = &notification;
    msg.size = sizeof(notification);

    bool received = false;
    std::string receivedEvent;

    QObject::connect(client, &IPCChannelQt::messageReceived,
                     [&](const Message& receivedMsg) {
        auto* ntf = static_cast<const KeyEventNotification*>(receivedMsg.data);
        receivedEvent = ntf->keyEvent;
        received = true;
    });

    server->send(msg);
    QTest::qWait(50);

    EXPECT_TRUE(received);
    EXPECT_EQ(u8"[12:34:56.789] 日本語キー ↓", receivedEvent)
        << "Unicode characters should be preserved";
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(IPCChannelQtTest, MessageLatencyUnder5ms) {
    const int iterations = 100;
    QElapsedTimer timer;
    qint64 totalNs = 0;

    for (int i = 0; i < iterations; i++) {
        bool received = false;

        QObject::connect(server, &IPCChannelQt::messageReceived,
                         [&](const Message&) {
            received = true;
        });

        Message msg;
        msg.type = CmdInvestigateWindow;
        msg.data = nullptr;
        msg.size = 0;

        timer.start();
        client->send(msg);

        // Wait for message to arrive
        int waitTime = 0;
        while (!received && waitTime < 50) {
            QTest::qWait(1);
            waitTime++;
        }

        totalNs += timer.nsecsElapsed();

        QObject::disconnect(server, &IPCChannelQt::messageReceived, nullptr, nullptr);
    }

    double avgMs = (totalNs / iterations) / 1000000.0;
    EXPECT_LT(avgMs, 5.0) << "Average latency should be <5ms, got " << avgMs << "ms";
}

TEST_F(IPCChannelQtTest, HighThroughputStressTest) {
    const int messageCount = 1000;
    std::atomic<int> receivedCount{0};

    QObject::connect(server, &IPCChannelQt::messageReceived,
                     [&](const Message&) {
        receivedCount++;
    });

    // Send many messages rapidly
    for (int i = 0; i < messageCount; i++) {
        Message msg;
        msg.type = static_cast<MessageType>(CmdInvestigateWindow + (i % 10));
        msg.data = nullptr;
        msg.size = 0;
        client->send(msg);
    }

    // Wait for all messages to arrive
    QTest::qWait(1000);

    EXPECT_EQ(messageCount, receivedCount.load())
        << "Should receive all messages in stress test";
}

// =============================================================================
// Reconnection Tests
// =============================================================================

TEST_F(IPCChannelQtTest, ReconnectAfterServerShutdown) {
    // Initial connection is established in SetUp()
    ASSERT_TRUE(client->isConnected());

    // Shutdown server
    delete server;
    server = nullptr;
    QTest::qWait(100);

    EXPECT_FALSE(client->isConnected()) << "Client should detect server shutdown";

    // Restart server
    server = new IPCChannelQt(serverName);
    server->listen();
    QTest::qWait(50);

    // Reconnect client
    client->connect(serverName);
    QTest::qWait(100);

    EXPECT_TRUE(client->isConnected()) << "Client should reconnect to new server";
}

// =============================================================================
// Main Function
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
