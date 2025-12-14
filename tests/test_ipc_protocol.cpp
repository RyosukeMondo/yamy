/**
 * @file test_ipc_protocol.cpp
 * @brief IPC protocol serialization/deserialization tests for GUI command set
 */

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTest>
#include <cstring>
#include <string>
#include <vector>

#include "core/ipc_messages.h"
#include "core/platform/linux/ipc_channel_qt.h"

using yamy::ipc::InvestigateWindowRequest;
using yamy::ipc::KeyEventNotification;
using yamy::ipc::Message;
using yamy::ipc::MessageType;
using yamy::platform::IPCChannelQt;

class IPCProtocolTest : public ::testing::Test {
protected:
    static QCoreApplication* app;

    static void SetUpTestSuite() {
        if (!QCoreApplication::instance()) {
            int argc = 0;
            app = new QCoreApplication(argc, nullptr);
        }
    }

    void SetUp() override {
        static int counter = 0;
        serverName = "ipc-protocol-" + std::to_string(counter++);

        server = new IPCChannelQt(serverName);
        client = new IPCChannelQt("ipc-protocol-client");

        server->listen();
        client->connect(serverName);
        QTest::qWait(50);
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

QCoreApplication* IPCProtocolTest::app = nullptr;

// Ensure numeric compatibility to guard against accidental enum changes
TEST_F(IPCProtocolTest, MessageIdsAreStable) {
    EXPECT_EQ(static_cast<uint32_t>(MessageType::CmdInvestigateWindow), 0x1001u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::RspInvestigateWindow), 0x1002u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::CmdEnableInvestigateMode), 0x1003u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::CmdDisableInvestigateMode), 0x1004u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::NtfKeyEvent), 0x1005u);

    EXPECT_EQ(static_cast<uint32_t>(MessageType::CmdReload), 0x2001u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::CmdStop), 0x2002u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::CmdStart), 0x2003u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::CmdGetStatus), 0x2004u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::CmdGetConfig), 0x2005u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::CmdGetKeymaps), 0x2006u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::CmdGetMetrics), 0x2007u);

    EXPECT_EQ(static_cast<uint32_t>(MessageType::RspOk), 0x2100u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::RspError), 0x2101u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::RspStatus), 0x2102u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::RspConfig), 0x2103u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::RspKeymaps), 0x2104u);
    EXPECT_EQ(static_cast<uint32_t>(MessageType::RspMetrics), 0x2105u);
}

TEST_F(IPCProtocolTest, ControlCommandRoundTripsPreservePayload) {
    struct CommandCase {
        MessageType type;
        std::string payload;
    };

    std::vector<CommandCase> cases = {
        {MessageType::CmdReload, "reload:mock"},
        {MessageType::CmdStop, ""},
        {MessageType::CmdStart, ""},
        {MessageType::CmdGetStatus, ""},
        {MessageType::CmdGetConfig, "active"},
        {MessageType::CmdGetKeymaps, ""},
        {MessageType::CmdGetMetrics, "latency-only"},
    };

    for (const auto& testCase : cases) {
        bool received = false;
        MessageType receivedType{};
        std::string receivedPayload;

        auto conn = QObject::connect(
            server, &IPCChannelQt::messageReceived,
            [&](const Message& msg) {
                receivedType = msg.type;
                receivedPayload.assign(static_cast<const char*>(msg.data),
                                       static_cast<size_t>(msg.size));
                received = true;
            });

        Message msg;
        msg.type = testCase.type;
        msg.data = testCase.payload.data();
        msg.size = testCase.payload.size();

        client->send(msg);
        QTest::qWait(50);

        QObject::disconnect(conn);

        EXPECT_TRUE(received) << "Message for type " << static_cast<uint32_t>(testCase.type)
                              << " should arrive";
        EXPECT_EQ(testCase.type, receivedType);
        EXPECT_EQ(testCase.payload, receivedPayload);
    }
}

TEST_F(IPCProtocolTest, ResponseRoundTripsPreserveJsonPayloads) {
    struct ResponseCase {
        MessageType type;
        std::string payload;
    };

    std::vector<ResponseCase> responses = {
        {MessageType::RspOk, "OK"},
        {MessageType::RspError, "Permission denied"},
        {MessageType::RspStatus, R"({"engine_running":true,"enabled":true})"},
        {MessageType::RspConfig, R"({"active_config":"mock.mayu"})"},
        {MessageType::RspKeymaps, R"({"keymaps":["mock","layered"]})"},
        {MessageType::RspMetrics, R"({"latency_ns":1024})"},
    };

    for (const auto& testCase : responses) {
        bool received = false;
        MessageType receivedType{};
        std::string receivedPayload;

        auto conn = QObject::connect(
            client, &IPCChannelQt::messageReceived,
            [&](const Message& msg) {
                receivedType = msg.type;
                receivedPayload.assign(static_cast<const char*>(msg.data),
                                       static_cast<size_t>(msg.size));
                received = true;
            });

        Message msg;
        msg.type = testCase.type;
        msg.data = testCase.payload.data();
        msg.size = testCase.payload.size();

        server->send(msg);
        QTest::qWait(50);

        QObject::disconnect(conn);

        EXPECT_TRUE(received) << "Response for type " << static_cast<uint32_t>(testCase.type)
                              << " should arrive";
        EXPECT_EQ(testCase.type, receivedType);
        EXPECT_EQ(testCase.payload, receivedPayload);
    }
}

TEST_F(IPCProtocolTest, InvestigateWindowRequestRoundTrip) {
    InvestigateWindowRequest request{};
    request.hwnd = reinterpret_cast<void*>(0xCAFEBABE);

    bool received = false;
    void* receivedHandle = nullptr;
    size_t receivedSize = 0;

    auto conn = QObject::connect(
        server, &IPCChannelQt::messageReceived,
        [&](const Message& msg) {
            receivedSize = msg.size;
            auto* req = static_cast<const InvestigateWindowRequest*>(msg.data);
            receivedHandle = req->hwnd;
            received = true;
        });

    Message msg{MessageType::CmdInvestigateWindow, &request, sizeof(request)};
    client->send(msg);
    QTest::qWait(50);

    QObject::disconnect(conn);

    EXPECT_TRUE(received);
    EXPECT_EQ(sizeof(request), receivedSize);
    EXPECT_EQ(request.hwnd, receivedHandle);
}

TEST_F(IPCProtocolTest, KeyEventNotificationRoundTrip) {
    KeyEventNotification notification{};
    std::string payload = "[12:00:00.000] Ctrl-Alt-K pressed";
    strncpy(notification.keyEvent, payload.c_str(), sizeof(notification.keyEvent) - 1);

    bool received = false;
    std::string receivedEvent;

    auto conn = QObject::connect(
        client, &IPCChannelQt::messageReceived,
        [&](const Message& msg) {
            auto* ntf = static_cast<const KeyEventNotification*>(msg.data);
            receivedEvent = ntf->keyEvent;
            received = true;
        });

    Message msg{MessageType::NtfKeyEvent, &notification, sizeof(notification)};
    server->send(msg);
    QTest::qWait(50);

    QObject::disconnect(conn);

    EXPECT_TRUE(received);
    EXPECT_EQ(payload, receivedEvent);
}

TEST_F(IPCProtocolTest, EmptyCommandHasZeroLengthPayload) {
    Message msg{};
    msg.type = MessageType::CmdEnableInvestigateMode;
    msg.data = nullptr;
    msg.size = 0;

    bool received = false;
    size_t receivedSize = 123;  // sentinel

    auto conn = QObject::connect(
        server, &IPCChannelQt::messageReceived,
        [&](const Message& receivedMsg) {
            receivedSize = receivedMsg.size;
            received = true;
        });

    client->send(msg);
    QTest::qWait(50);

    QObject::disconnect(conn);

    EXPECT_TRUE(received);
    EXPECT_EQ(0u, receivedSize);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
