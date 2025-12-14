/**
 * @file test_ipc_protocol.cpp
 * @brief IPC protocol serialization/deserialization tests for GUI command set
 */

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTest>
#include <algorithm>
#include <functional>
#include <cstring>
#include <string>
#include <vector>

#include "core/ipc_messages.h"
#include "core/platform/ipc_defs.h"
#include "core/platform/linux/ipc_channel_qt.h"

using yamy::ipc::InvestigateWindowRequest;
using yamy::ipc::KeyEventNotification;
using yamy::ipc::Message;
using yamy::ipc::MessageType;
using yamy::platform::IPCChannelQt;
using GuiMessageType = yamy::MessageType;
using yamy::CmdReloadConfigRequest;
using yamy::CmdSetEnabledRequest;
using yamy::CmdSwitchConfigRequest;
using yamy::RspConfigListPayload;
using yamy::RspStatusPayload;

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

namespace {
yamy::ipc::MessageType toWireType(GuiMessageType type) {
    return static_cast<yamy::ipc::MessageType>(static_cast<uint32_t>(type));
}

template <size_t N>
std::string toString(const std::array<char, N>& buffer) {
    auto end = std::find(buffer.begin(), buffer.end(), '\0');
    return std::string(buffer.begin(), end);
}

template <size_t N>
void copyString(const std::string& value, std::array<char, N>& buffer) {
    std::fill(buffer.begin(), buffer.end(), '\0');
    const auto copyLen = std::min(value.size(), buffer.size() - 1);
    std::copy_n(value.c_str(), copyLen, buffer.data());
}
} // namespace

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

TEST_F(IPCProtocolTest, GuiMessageIdsAreStable) {
    EXPECT_EQ(static_cast<uint32_t>(GuiMessageType::CmdGetStatus), 0x5001u);
    EXPECT_EQ(static_cast<uint32_t>(GuiMessageType::CmdSetEnabled), 0x5002u);
    EXPECT_EQ(static_cast<uint32_t>(GuiMessageType::CmdSwitchConfig), 0x5003u);
    EXPECT_EQ(static_cast<uint32_t>(GuiMessageType::CmdReloadConfig), 0x5004u);
    EXPECT_EQ(static_cast<uint32_t>(GuiMessageType::RspStatus), 0x5101u);
    EXPECT_EQ(static_cast<uint32_t>(GuiMessageType::RspConfigList), 0x5102u);
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

TEST_F(IPCProtocolTest, GuiCommandStructRoundTripsPreservePayloads) {
    CmdSetEnabledRequest enableRequest{};
    enableRequest.enabled = true;

    CmdSwitchConfigRequest switchRequest{};
    copyString("gui-config.mayu", switchRequest.configName);

    CmdReloadConfigRequest reloadRequest{};
    copyString("reload.mayu", reloadRequest.configName);

    struct CommandCase {
        GuiMessageType type;
        const void* data;
        size_t size;
        std::function<void(const Message&)> validate;
    };

    std::vector<CommandCase> cases = {
        {GuiMessageType::CmdSetEnabled,
         &enableRequest,
         sizeof(enableRequest),
         [&](const Message& msg) {
             ASSERT_EQ(sizeof(CmdSetEnabledRequest), msg.size);
             auto* payload = static_cast<const CmdSetEnabledRequest*>(msg.data);
             EXPECT_TRUE(payload->enabled);
         }},
        {GuiMessageType::CmdSwitchConfig,
         &switchRequest,
         sizeof(switchRequest),
         [&](const Message& msg) {
             ASSERT_EQ(sizeof(CmdSwitchConfigRequest), msg.size);
             auto* payload = static_cast<const CmdSwitchConfigRequest*>(msg.data);
             EXPECT_EQ("gui-config.mayu", toString(payload->configName));
         }},
        {GuiMessageType::CmdReloadConfig,
         &reloadRequest,
         sizeof(reloadRequest),
         [&](const Message& msg) {
             ASSERT_EQ(sizeof(CmdReloadConfigRequest), msg.size);
             auto* payload = static_cast<const CmdReloadConfigRequest*>(msg.data);
             EXPECT_EQ("reload.mayu", toString(payload->configName));
         }},
    };

    for (const auto& testCase : cases) {
        bool received = false;

        auto conn = QObject::connect(
            server, &IPCChannelQt::messageReceived,
            [&](const Message& msg) {
                received = true;
                EXPECT_EQ(toWireType(testCase.type), msg.type);
                testCase.validate(msg);
            });

        Message msg{toWireType(testCase.type), testCase.data, testCase.size};
        client->send(msg);
        QTest::qWait(50);

        QObject::disconnect(conn);
        EXPECT_TRUE(received);
    }
}

TEST_F(IPCProtocolTest, GuiStatusResponseRoundTrip) {
    RspStatusPayload status{};
    status.engineRunning = true;
    status.enabled = false;
    copyString("active.mayu", status.activeConfig);
    copyString("last-error", status.lastError);

    bool received = false;
    RspStatusPayload receivedPayload{};

    auto conn = QObject::connect(
        client, &IPCChannelQt::messageReceived,
        [&](const Message& msg) {
            received = true;
            ASSERT_EQ(sizeof(RspStatusPayload), msg.size);
            auto* payload = static_cast<const RspStatusPayload*>(msg.data);
            receivedPayload = *payload;
        });

    Message msg{toWireType(GuiMessageType::RspStatus), &status, sizeof(status)};
    server->send(msg);
    QTest::qWait(50);

    QObject::disconnect(conn);

    EXPECT_TRUE(received);
    EXPECT_TRUE(receivedPayload.engineRunning);
    EXPECT_FALSE(receivedPayload.enabled);
    EXPECT_EQ("active.mayu", toString(receivedPayload.activeConfig));
    EXPECT_EQ("last-error", toString(receivedPayload.lastError));
}

TEST_F(IPCProtocolTest, GuiConfigListResponseRoundTrip) {
    RspConfigListPayload configList{};
    configList.count = 2;
    copyString("first.mayu", configList.configs[0]);
    copyString("second.mayu", configList.configs[1]);

    bool received = false;
    RspConfigListPayload receivedPayload{};

    auto conn = QObject::connect(
        client, &IPCChannelQt::messageReceived,
        [&](const Message& msg) {
            received = true;
            ASSERT_EQ(sizeof(RspConfigListPayload), msg.size);
            auto* payload = static_cast<const RspConfigListPayload*>(msg.data);
            receivedPayload = *payload;
        });

    Message msg{toWireType(GuiMessageType::RspConfigList), &configList, sizeof(configList)};
    server->send(msg);
    QTest::qWait(50);

    QObject::disconnect(conn);

    EXPECT_TRUE(received);
    EXPECT_EQ(2u, receivedPayload.count);
    EXPECT_EQ("first.mayu", toString(receivedPayload.configs[0]));
    EXPECT_EQ("second.mayu", toString(receivedPayload.configs[1]));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
