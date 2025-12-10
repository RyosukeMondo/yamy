#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QApplication>
#include "ui/qt/dialog_investigate_qt.h"
#include "ui/qt/crosshair_widget_qt.h"
#include "core/platform/window_system_interface.h"
#include "core/platform/ipc_channel_interface.h"
#include "core/ipc_messages.h"

// Mock for IWindowSystem
class MockWindowSystem : public yamy::platform::IWindowSystem {
public:
    MOCK_METHOD(yamy::platform::WindowHandle, getForegroundWindow, (), (override));
    MOCK_METHOD(std::string, getWindowText, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(std::string, getClassName, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(bool, getWindowRect, (yamy::platform::WindowHandle, yamy::platform::Rect*), (override));
    MOCK_METHOD(yamy::platform::WindowShowCmd, getShowCommand, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(uint32_t, getWindowProcessId, (yamy::platform::WindowHandle), (override));
};

// Mock for IIPCChannel
class MockIPCChannel : public yamy::platform::IIPCChannel {
public:
    MOCK_METHOD(void, connect, (const std::string&), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(void, listen, (), (override));
    MOCK_METHOD(bool, isConnected, (), (override));
    MOCK_METHOD(void, send, (const yamy::ipc::Message&), (override));
};

class DialogInvestigateTest : public ::testing::Test {
protected:
    static QApplication* app;

    static void SetUpTestSuite() {
        if (!QApplication::instance()) {
            int argc = 0;
            app = new QApplication(argc, nullptr);
        }
    }

    void SetUp() override {
        dialog = new DialogInvestigateQt();
        mockWindowSystem = new MockWindowSystem();
        mockIpcChannel = new MockIPCChannel();
        // The dialog takes ownership of the pointers
        dialog->setWindowSystem(std::unique_ptr<yamy::platform::IWindowSystem>(mockWindowSystem));
        dialog->setIpcChannel(std::unique_ptr<yamy::platform::IIPCChannel>(mockIpcChannel));
    }

    void TearDown() override {
        delete dialog;
    }

    DialogInvestigateQt* dialog;
    MockWindowSystem* mockWindowSystem;
    MockIPCChannel* mockIpcChannel;
};

QApplication* DialogInvestigateTest::app = nullptr;

TEST_F(DialogInvestigateTest, WindowSelectionUpdatesInfoWindow) {
    yamy::platform::WindowHandle fakeHandle = reinterpret_cast<yamy::platform::WindowHandle>(12345);
    EXPECT_CALL(*mockWindowSystem, getWindowText(fakeHandle)).WillOnce(::testing::Return("Test Window"));
    EXPECT_CALL(*mockWindowSystem, getClassName(fakeHandle)).WillOnce(::testing::Return("TestClass"));

    dialog->onWindowSelected(fakeHandle);

    ASSERT_EQ(dialog->findChild<QLabel*>("m_labelTitle")->text().toStdString(), "Test Window");
    ASSERT_EQ(dialog->findChild<QLabel*>("m_labelClass")->text().toStdString(), "TestClass");
}

TEST_F(DialogInvestigateTest, KeymapQueryIsSentViaIpc) {
    yamy::platform::WindowHandle fakeHandle = reinterpret_cast<yamy::platform::WindowHandle>(54321);
    EXPECT_CALL(*mockIpcChannel, send(::testing::_))
        .WillOnce([](const yamy::ipc::Message& msg) {
            ASSERT_EQ(msg.type, yamy::ipc::CmdInvestigateWindow);
            const auto* request = static_cast<const yamy::ipc::InvestigateWindowRequest*>(msg.data);
            ASSERT_EQ(request->hwnd, reinterpret_cast<yamy::platform::WindowHandle>(54321));
        });

    dialog->onWindowSelected(fakeHandle);
}

// Add more tests for live log, clipboard, etc.

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
