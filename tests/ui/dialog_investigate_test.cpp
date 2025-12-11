#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QApplication>
#include "ui/qt/dialog_investigate_qt.h"
#include "ui/qt/crosshair_widget_qt.h"
#include "core/platform/window_system_interface.h"
#include "core/platform/ipc_channel_interface.h"
#include "core/ipc_messages.h"

// Mock for IWindowSystem - complete mock with all pure virtual methods
class MockWindowSystem : public yamy::platform::IWindowSystem {
public:
    MOCK_METHOD(yamy::platform::WindowHandle, getForegroundWindow, (), (override));
    MOCK_METHOD(yamy::platform::WindowHandle, windowFromPoint, (const yamy::platform::Point&), (override));
    MOCK_METHOD(bool, getWindowRect, (yamy::platform::WindowHandle, yamy::platform::Rect*), (override));
    MOCK_METHOD(std::string, getWindowText, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(std::string, getClassName, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(std::string, getTitleName, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(uint32_t, getWindowThreadId, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(uint32_t, getWindowProcessId, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(bool, setForegroundWindow, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(bool, moveWindow, (yamy::platform::WindowHandle, const yamy::platform::Rect&), (override));
    MOCK_METHOD(bool, showWindow, (yamy::platform::WindowHandle, int), (override));
    MOCK_METHOD(bool, closeWindow, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(yamy::platform::WindowHandle, getParent, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(bool, isMDIChild, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(bool, isChild, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(yamy::platform::WindowShowCmd, getShowCommand, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(bool, isConsoleWindow, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(void, getCursorPos, (yamy::platform::Point*), (override));
    MOCK_METHOD(void, setCursorPos, (const yamy::platform::Point&), (override));
    MOCK_METHOD(int, getMonitorCount, (), (override));
    MOCK_METHOD(bool, getMonitorRect, (int, yamy::platform::Rect*), (override));
    MOCK_METHOD(bool, getMonitorWorkArea, (int, yamy::platform::Rect*), (override));
    MOCK_METHOD(int, getMonitorIndex, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(int, getSystemMetrics, (yamy::platform::SystemMetric), (override));
    MOCK_METHOD(bool, getWorkArea, (yamy::platform::Rect*), (override));
    MOCK_METHOD(std::string, getClipboardText, (), (override));
    MOCK_METHOD(bool, setClipboardText, (const std::string&), (override));
    MOCK_METHOD(bool, getClientRect, (yamy::platform::WindowHandle, yamy::platform::Rect*), (override));
    MOCK_METHOD(bool, getChildWindowRect, (yamy::platform::WindowHandle, yamy::platform::Rect*), (override));
    MOCK_METHOD(unsigned int, mapVirtualKey, (unsigned int), (override));
    MOCK_METHOD(bool, postMessage, (yamy::platform::WindowHandle, unsigned int, uintptr_t, intptr_t), (override));
    MOCK_METHOD(unsigned int, registerWindowMessage, (const std::string&), (override));
    MOCK_METHOD(bool, sendMessageTimeout, (yamy::platform::WindowHandle, unsigned int, uintptr_t, intptr_t, unsigned int, unsigned int, uintptr_t*), (override));
    MOCK_METHOD(bool, sendCopyData, (yamy::platform::WindowHandle, yamy::platform::WindowHandle, const yamy::platform::CopyData&, uint32_t, uint32_t, uintptr_t*), (override));
    MOCK_METHOD(bool, setWindowZOrder, (yamy::platform::WindowHandle, yamy::platform::ZOrder), (override));
    MOCK_METHOD(bool, isWindowTopMost, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(bool, isWindowLayered, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(bool, setWindowLayered, (yamy::platform::WindowHandle, bool), (override));
    MOCK_METHOD(bool, setLayeredWindowAttributes, (yamy::platform::WindowHandle, unsigned long, unsigned char, unsigned long), (override));
    MOCK_METHOD(bool, redrawWindow, (yamy::platform::WindowHandle), (override));
    MOCK_METHOD(bool, enumerateWindows, (yamy::platform::IWindowSystem::WindowEnumCallback), (override));
    MOCK_METHOD(int, shellExecute, (const std::string&, const std::string&, const std::string&, const std::string&, int), (override));
    MOCK_METHOD(bool, disconnectNamedPipe, (void*), (override));
    MOCK_METHOD(bool, connectNamedPipe, (void*, void*), (override));
    MOCK_METHOD(bool, writeFile, (void*, const void*, unsigned int, unsigned int*, void*), (override));
    MOCK_METHOD(void*, openMutex, (const std::string&), (override));
    MOCK_METHOD(void*, openFileMapping, (const std::string&), (override));
    MOCK_METHOD(void*, mapViewOfFile, (void*), (override));
    MOCK_METHOD(bool, unmapViewOfFile, (void*), (override));
    MOCK_METHOD(void, closeHandle, (void*), (override));
    MOCK_METHOD(void*, loadLibrary, (const std::string&), (override));
    MOCK_METHOD(void*, getProcAddress, (void*, const std::string&), (override));
    MOCK_METHOD(bool, freeLibrary, (void*), (override));
    MOCK_METHOD(yamy::platform::WindowHandle, getToplevelWindow, (yamy::platform::WindowHandle, bool*), (override));
    MOCK_METHOD(bool, changeMessageFilter, (uint32_t, uint32_t), (override));
};

// Mock for IIPCChannel - complete mock with all pure virtual methods
class MockIPCChannel : public yamy::platform::IIPCChannel {
    Q_OBJECT
public:
    MOCK_METHOD(void, connect, (const std::string&), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(void, listen, (), (override));
    MOCK_METHOD(bool, isConnected, (), (override));
    MOCK_METHOD(void, send, (const yamy::ipc::Message&), (override));
    MOCK_METHOD(std::unique_ptr<yamy::ipc::Message>, nonBlockingReceive, (), (override));
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

// Basic test - dialog can be created and destroyed
TEST_F(DialogInvestigateTest, DialogCreation) {
    ASSERT_NE(dialog, nullptr);
}

// Test that the dialog can show
TEST_F(DialogInvestigateTest, DialogShowsWithoutCrash) {
    dialog->show();
    dialog->hide();
    SUCCEED();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
