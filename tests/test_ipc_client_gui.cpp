/**
 * @file test_ipc_client_gui.cpp
 * @brief End-to-end IPCClientGUI tests against the mock IPC server.
 */

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <QTest>
#include <algorithm>
#include <memory>
#include <string>

#include "ui/qt/ipc_client_gui.h"

namespace {
template <size_t N>
std::string toString(const std::array<char, N>& buffer) {
    auto end = std::find(buffer.begin(), buffer.end(), '\0');
    return std::string(buffer.begin(), end);
}
}  // namespace

class IPCClientGUITest : public ::testing::Test {
protected:
    static QCoreApplication* app;

    static void SetUpTestSuite() {
        if (!QCoreApplication::instance()) {
            int argc = 0;
            app = new QCoreApplication(argc, nullptr);
        }
    }

    void TearDown() override {
        if (mockServer.state() != QProcess::NotRunning) {
            mockServer.kill();
            mockServer.waitForFinished(2000);
        }
        client.reset();
        QCoreApplication::processEvents();
    }

    QString mockServerPath() const {
        return QDir(QCoreApplication::applicationDirPath())
            .absoluteFilePath(QStringLiteral("yamy_mock_ipc_server"));
    }

    bool startMockServer(const QString& socketName) {
        QStringList args;
        args << QStringLiteral("--socket-name") << socketName;
        mockServer.start(mockServerPath(), args);
        return mockServer.waitForStarted(3000);
    }

    QProcess mockServer;
    std::unique_ptr<IPCClientGUI> client;
};

QCoreApplication* IPCClientGUITest::app = nullptr;

TEST_F(IPCClientGUITest, SendsCommandsAndEmitsSignalsFromMockServer) {
    const QString socketName = QStringLiteral("ipc-client-gui-%1-%2")
                                   .arg(QCoreApplication::applicationPid())
                                   .arg(QDateTime::currentMSecsSinceEpoch());

    ASSERT_TRUE(startMockServer(socketName)) << "Mock server failed to start";

    client = std::make_unique<IPCClientGUI>();

    bool isConnected = false;
    bool disconnectedSeen = false;
    QObject::connect(client.get(), &IPCClientGUI::connectionStateChanged,
                     [&](bool connected) {
                         isConnected = connected;
                         if (!connected) {
                             disconnectedSeen = true;
                         }
                     });

    yamy::RspStatusPayload status{};
    yamy::RspConfigListPayload configs{};
    bool statusReceived = false;
    bool configsReceived = false;

    QObject::connect(client.get(), &IPCClientGUI::statusReceived,
                     [&](const yamy::RspStatusPayload& payload) {
                         status = payload;
                         statusReceived = true;
                     });
    QObject::connect(client.get(), &IPCClientGUI::configListReceived,
                     [&](const yamy::RspConfigListPayload& payload) {
                         configs = payload;
                         configsReceived = true;
                     });

    client->connectToDaemon(socketName.toStdString());
    QTRY_VERIFY_WITH_TIMEOUT(isConnected, 2000);

    client->sendGetStatus();
    QTRY_VERIFY_WITH_TIMEOUT(statusReceived, 2000);
    QTRY_VERIFY_WITH_TIMEOUT(configsReceived, 2000);

    EXPECT_TRUE(status.engineRunning);
    EXPECT_TRUE(status.enabled);
    EXPECT_EQ("mock.mayu", toString(status.activeConfig));
    EXPECT_EQ(2u, configs.count);

    statusReceived = false;
    configsReceived = false;
    client->sendSetEnabled(false);
    QTRY_VERIFY_WITH_TIMEOUT(statusReceived, 2000);
    EXPECT_FALSE(status.enabled);

    statusReceived = false;
    configsReceived = false;
    client->sendSwitchConfig(QStringLiteral("layered.mayu"));
    QTRY_VERIFY_WITH_TIMEOUT(statusReceived, 2000);
    EXPECT_EQ("layered.mayu", toString(status.activeConfig));

    statusReceived = false;
    configsReceived = false;
    client->sendReloadConfig(QStringLiteral("mock.mayu"));
    QTRY_VERIFY_WITH_TIMEOUT(statusReceived, 2000);
    EXPECT_EQ("mock.mayu", toString(status.activeConfig));

    client->disconnectFromDaemon();
    QTRY_VERIFY_WITH_TIMEOUT(!isConnected, 2000);
    EXPECT_TRUE(disconnectedSeen);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
