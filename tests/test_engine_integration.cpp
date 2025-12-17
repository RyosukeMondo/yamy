#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <iostream>

#include "../../src/core/engine/engine.h"
#include "../../src/core/platform/window_system_interface.h"
#include "../../src/core/platform/input_injector_interface.h"
#include "../../src/core/platform/input_hook_interface.h"
#include "../../src/core/platform/input_driver_interface.h"
#include "../../src/utils/msgstream.h"
#include "../../src/core/input/keyboard.h"

using namespace yamy::platform;

// --- Manual Mocks ---

class MockWindowSystem : public IWindowSystem {
public:
    WindowHandle getForegroundWindow() override { return nullptr; }
    WindowHandle windowFromPoint(const Point&) override { return nullptr; }
    bool getWindowRect(WindowHandle, Rect*) override { return false; }
    std::string getWindowText(WindowHandle) override { return ""; }
    std::string getClassName(WindowHandle) override { return "MockWindowClass"; }
    std::string getTitleName(WindowHandle) override { return "MockTitle"; }
    uint32_t getWindowThreadId(WindowHandle) override { return 1; }
    uint32_t getWindowProcessId(WindowHandle) override { return 1; }
    bool setForegroundWindow(WindowHandle) override { return true; }
    bool moveWindow(WindowHandle, const Rect&) override { return true; }
    bool showWindow(WindowHandle, int) override { return true; }
    bool closeWindow(WindowHandle) override { return true; }
    WindowHandle getParent(WindowHandle) override { return nullptr; }
    bool isMDIChild(WindowHandle) override { return false; }
    bool isChild(WindowHandle) override { return false; }
    WindowShowCmd getShowCommand(WindowHandle) override { return WindowShowCmd::Normal; }
    bool isConsoleWindow(WindowHandle) override { return false; }
    void getCursorPos(Point*) override {}
    void setCursorPos(const Point&) override {}
    int getMonitorCount() override { return 1; }
    bool getMonitorRect(int, Rect*) override { return false; }
    bool getMonitorWorkArea(int, Rect*) override { return false; }
    int getMonitorIndex(WindowHandle) override { return 0; }
    int getSystemMetrics(SystemMetric) override { return 0; }
    bool getWorkArea(Rect*) override { return false; }
    std::string getClipboardText() override { return ""; }
    bool setClipboardText(const std::string&) override { return true; }
    bool getClientRect(WindowHandle, Rect*) override { return false; }
    bool getChildWindowRect(WindowHandle, Rect*) override { return false; }
    unsigned int mapVirtualKey(unsigned int) override { return 0; }
    bool postMessage(WindowHandle, unsigned int, uintptr_t, intptr_t) override { return true; }
    unsigned int registerWindowMessage(const std::string&) override { return 0; }
    bool sendMessageTimeout(WindowHandle, unsigned int, uintptr_t, intptr_t, unsigned int, unsigned int, uintptr_t*) override { return true; }
    bool sendCopyData(WindowHandle, WindowHandle, const CopyData&, uint32_t, uint32_t, uintptr_t*) override { return true; }
    bool setWindowZOrder(WindowHandle, ZOrder) override { return true; }
    bool isWindowTopMost(WindowHandle) override { return false; }
    bool isWindowLayered(WindowHandle) override { return false; }
    bool setWindowLayered(WindowHandle, bool) override { return true; }
    bool setLayeredWindowAttributes(WindowHandle, unsigned long, unsigned char, unsigned long) override { return true; }
    bool redrawWindow(WindowHandle) override { return true; }
    bool enumerateWindows(WindowEnumCallback) override { return true; }
    int shellExecute(const std::string&, const std::string&, const std::string&, const std::string&, int) override { return 0; }
    bool disconnectNamedPipe(void*) override { return true; }
    bool connectNamedPipe(void*, void*) override { return true; }
    bool writeFile(void*, const void*, unsigned int, unsigned int*, void*) override { return true; }
    void* openMutex(const std::string&) override { return nullptr; }
    void* openFileMapping(const std::string&) override { return nullptr; }
    void* mapViewOfFile(void*) override { return nullptr; }
    bool unmapViewOfFile(void*) override { return true; }
    void closeHandle(void*) override {}
    void* loadLibrary(const std::string&) override { return nullptr; }
    void* getProcAddress(void*, const std::string&) override { return nullptr; }
    bool freeLibrary(void*) override { return true; }
    WindowHandle getToplevelWindow(WindowHandle, bool*) override { return nullptr; }
    bool changeMessageFilter(uint32_t, uint32_t) override { return true; }
};

class MockInputInjector : public IInputInjector {
public:
    void inject(const KEYBOARD_INPUT_DATA *data, const InjectionContext &ctx, const void *rawData) override {
        std::cout << "[MockInjector] inject called: MakeCode=0x" << std::hex << data->MakeCode << std::dec << std::endl;
        lastMakeCode = data->MakeCode;
        injectCallCount++;
    }
    void keyDown(KeyCode) override {}
    void keyUp(KeyCode) override {}
    void mouseMove(int32_t, int32_t) override {}
    void mouseButton(MouseButton, bool) override {}
    void mouseWheel(int32_t) override {}

    uint16_t lastMakeCode = 0;
    int injectCallCount = 0;
};

class MockInputHook : public IInputHook {
public:
    bool install(KeyCallback keyCallback, MouseCallback mouseCallback) override {
        capturedKeyCallback = keyCallback;
        return true;
    }
    void uninstall() override {}
    bool isInstalled() const override { return true; }

    KeyCallback capturedKeyCallback;
};

class MockInputDriver : public IInputDriver {
public:
    bool open(void*) override { return true; }
    void close() override {}
    void manageExtension(const std::string&, const std::string&, bool, void**) override {}
};

// --- Test Fixture ---

class EngineIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        logStream = new tomsgstream(0);
        
        mockWindowSystem = new MockWindowSystem();
        mockInputInjector = new MockInputInjector();
        mockInputHook = new MockInputHook();
        mockInputDriver = new MockInputDriver();
        
        engine = new Engine(*logStream, mockWindowSystem, nullptr, mockInputInjector, mockInputHook, mockInputDriver);
    }

    void TearDown() override {
        engine->stop();
        delete engine;
        delete logStream;
        
        delete mockWindowSystem;
        delete mockInputInjector;
        delete mockInputHook;
        delete mockInputDriver;
    }

    tomsgstream* logStream;
    MockWindowSystem* mockWindowSystem;
    MockInputInjector* mockInputInjector;
    MockInputHook* mockInputHook;
    MockInputDriver* mockInputDriver;
    Engine* engine;
};

// --- Tests ---

TEST_F(EngineIntegrationTest, SimpleSubstitution_A_to_Tab) {
    // 1. Setup minimal configuration
    Setting* setting = new Setting;
    
    // Create keys: A (0x1E) and Tab (0x0F)
    Key keyA; keyA.addName("A"); keyA.addScanCode(ScanCode(0x1E, 0));
    Key keyTab; keyTab.addName("Tab"); keyTab.addScanCode(ScanCode(0x0F, 0));
    
    setting->m_keyboard.addKey(keyA);
    setting->m_keyboard.addKey(keyTab);
    
    // Add substitute: A -> Tab
    // Search using non-alias name directly from the key we just added
    Key* pKeyA = setting->m_keyboard.searchKey("A");
    Key* pKeyTab = setting->m_keyboard.searchKey("Tab");
    
    ASSERT_NE(pKeyA, nullptr);
    ASSERT_NE(pKeyTab, nullptr);
    
    ModifiedKey mkFrom(pKeyA);
    ModifiedKey mkTo(pKeyTab);
    setting->m_keyboard.addSubstitute(mkFrom, mkTo);
    
    // Create a default Keymap (global keymap)
    Keymap* keymap = new Keymap("Global", nullptr, nullptr);
    setting->m_keymaps.add(*keymap);
    
    // 2. Start Engine
    engine->start();
    
    // 3. Apply Setting
    engine->setSetting(setting);
    
    // 4. Inject Event A (0x1E)
    ASSERT_TRUE(mockInputHook->capturedKeyCallback) << "InputHook::install callback not captured";
    
    KeyEvent event;
    event.scanCode = 0x1E; // A
    event.isKeyDown = true;
    event.extraInfo = 0;
    
    std::cout << "[TEST] Pushing event A (0x1E)..." << std::endl;
    mockInputHook->capturedKeyCallback(event);
    
    // 5. Wait for processing
    int retries = 10;
    while (retries-- > 0 && mockInputInjector->injectCallCount == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // 6. Verify
    EXPECT_EQ(mockInputInjector->injectCallCount, 1);
    EXPECT_EQ(mockInputInjector->lastMakeCode, 0x0F); // Expect Tab
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}