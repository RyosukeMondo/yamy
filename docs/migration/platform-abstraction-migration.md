# Platform Abstraction Migration Guide

This guide walks through migrating code from direct Win32 API usage to the platform abstraction layer (PAL), enabling cross-platform support for Windows and Linux.

## Overview

The platform abstraction layer provides a unified interface that encapsulates platform-specific operations. Code written against the PAL works on both Windows (Win32) and Linux (X11/evdev) without modification.

**Key Benefits:**
- Write once, run on both platforms
- Consistent error handling across platforms
- Platform-specific details hidden behind clean interfaces
- Easier testing through mockable interfaces

## Migration Process

### Step 1: Identify Win32 Dependencies

Before migrating, audit your code for direct Win32 API calls:

```bash
# Search for common Win32 patterns
grep -rn "HWND\|GetForegroundWindow\|SetWindowsHookEx\|SendInput" src/
grep -rn "#include <windows.h>" src/
```

Common Win32 APIs that need migration:

| Win32 API | Platform Abstraction |
|-----------|---------------------|
| `HWND` | `WindowHandle` |
| `GetForegroundWindow()` | `IWindowSystem::getForegroundWindow()` |
| `GetWindowTextW()` | `IWindowSystem::getWindowText()` |
| `GetClassName()` | `IWindowSystem::getClassName()` |
| `SetWindowsHookEx()` | `IInputHook::install()` |
| `SendInput()` | `IInputInjector::keyDown()`, `keyUp()`, etc. |
| `POINT`, `RECT` | `Point`, `Rect` |

### Step 2: Include Platform Headers

Replace Windows headers with platform abstraction headers:

```cpp
// Before (Win32)
#include <windows.h>

// After (Platform Abstraction)
#include "core/platform/window_system_interface.h"
#include "core/platform/input_hook_interface.h"
#include "core/platform/input_injector_interface.h"
#include "core/platform/types.h"
```

### Step 3: Use Platform Types

Replace Win32 types with platform-agnostic equivalents:

```cpp
// Before (Win32)
HWND hwnd = GetForegroundWindow();
RECT rect;
GetWindowRect(hwnd, &rect);
POINT pt;
GetCursorPos(&pt);

// After (Platform Abstraction)
using namespace yamy::platform;

WindowHandle hwnd = windowSystem->getForegroundWindow();
Rect rect;
windowSystem->getWindowRect(hwnd, &rect);
Point pt;
windowSystem->getCursorPos(&pt);
```

### Step 4: Inject Dependencies

Instead of calling Win32 APIs directly, inject platform interfaces via constructor or setter:

```cpp
// Before (Win32 - tightly coupled)
class MyComponent {
public:
    void doWork() {
        HWND fg = GetForegroundWindow();
        // ...
    }
};

// After (Platform Abstraction - loosely coupled)
class MyComponent {
public:
    explicit MyComponent(yamy::platform::IWindowSystem* windowSystem)
        : m_windowSystem(windowSystem) {}

    void doWork() {
        auto fg = m_windowSystem->getForegroundWindow();
        // ...
    }

private:
    yamy::platform::IWindowSystem* m_windowSystem;
};
```

## Before/After Examples

### Example 1: Window Information Query

**Before (Win32):**
```cpp
#include <windows.h>
#include <string>

std::string getActiveWindowInfo() {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return "";

    wchar_t title[256];
    GetWindowTextW(hwnd, title, 256);

    wchar_t className[256];
    GetClassNameW(hwnd, className, 256);

    // Manual UTF-16 to UTF-8 conversion needed
    int len = WideCharToMultiByte(CP_UTF8, 0, title, -1, nullptr, 0, nullptr, nullptr);
    std::string titleStr(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, title, -1, &titleStr[0], len, nullptr, nullptr);

    return titleStr;
}
```

**After (Platform Abstraction):**
```cpp
#include "core/platform/window_system_interface.h"

std::string getActiveWindowInfo(yamy::platform::IWindowSystem* ws) {
    auto hwnd = ws->getForegroundWindow();
    if (!hwnd) return "";

    // Returns UTF-8 directly - no manual conversion needed
    return ws->getWindowText(hwnd);
}
```

### Example 2: Keyboard Hook Installation

**Before (Win32):**
```cpp
#include <windows.h>

HHOOK g_keyboardHook = nullptr;

LRESULT CALLBACK keyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* kbd = (KBDLLHOOKSTRUCT*)lParam;

        if (wParam == WM_KEYDOWN && kbd->vkCode == VK_ESCAPE) {
            // Handle escape key
            return 1;  // Consume event
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

void installHook() {
    g_keyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        keyboardProc,
        GetModuleHandle(nullptr),
        0
    );
}

void uninstallHook() {
    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }
}
```

**After (Platform Abstraction):**
```cpp
#include "core/platform/input_hook_interface.h"
#include <memory>

std::unique_ptr<yamy::platform::IInputHook> g_inputHook;

void installHook() {
    using namespace yamy::platform;

    g_inputHook.reset(createInputHook());

    g_inputHook->install(
        [](const KeyEvent& ev) -> bool {
            // VK_ESCAPE scancode is 0x01
            if (ev.isKeyDown && ev.scanCode == 0x01) {
                // Handle escape key
                return true;  // Consume event
            }
            return false;  // Pass through
        },
        nullptr  // No mouse callback
    );
}

void uninstallHook() {
    if (g_inputHook) {
        g_inputHook->uninstall();
        g_inputHook.reset();
    }
}
```

### Example 3: Key Injection

**Before (Win32):**
```cpp
#include <windows.h>

void sendKey(WORD vk, bool keyDown) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = keyDown ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void typeChar() {
    sendKey('A', true);   // Key down
    sendKey('A', false);  // Key up
}
```

**After (Platform Abstraction):**
```cpp
#include "core/platform/input_injector_interface.h"
#include <memory>

std::unique_ptr<yamy::platform::IInputInjector> g_injector;

void initializeInjector(yamy::platform::IWindowSystem* ws) {
    g_injector.reset(yamy::platform::createInputInjector(ws));
}

void typeChar() {
    // KeyCode::Unknown uses scancode-based injection
    g_injector->keyDown(yamy::platform::KeyCode::Unknown);
    g_injector->keyUp(yamy::platform::KeyCode::Unknown);
}
```

### Example 4: Window Manipulation

**Before (Win32):**
```cpp
#include <windows.h>

void maximizeWindow(HWND hwnd) {
    ShowWindow(hwnd, SW_MAXIMIZE);
}

void moveWindowTo(HWND hwnd, int x, int y, int width, int height) {
    MoveWindow(hwnd, x, y, width, height, TRUE);
}

void bringToFront(HWND hwnd) {
    SetForegroundWindow(hwnd);
}
```

**After (Platform Abstraction):**
```cpp
#include "core/platform/window_system_interface.h"

void maximizeWindow(yamy::platform::IWindowSystem* ws,
                    yamy::platform::WindowHandle hwnd) {
    ws->showWindow(hwnd, 3);  // SW_MAXIMIZE equivalent
}

void moveWindowTo(yamy::platform::IWindowSystem* ws,
                  yamy::platform::WindowHandle hwnd,
                  int x, int y, int width, int height) {
    yamy::platform::Rect rect(x, y, x + width, y + height);
    ws->moveWindow(hwnd, rect);
}

void bringToFront(yamy::platform::IWindowSystem* ws,
                  yamy::platform::WindowHandle hwnd) {
    ws->setForegroundWindow(hwnd);
}
```

### Example 5: Clipboard Operations

**Before (Win32):**
```cpp
#include <windows.h>
#include <string>

std::string getClipboardText() {
    if (!OpenClipboard(nullptr)) return "";

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) {
        CloseClipboard();
        return "";
    }

    wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
    if (!pszText) {
        CloseClipboard();
        return "";
    }

    // Convert UTF-16 to UTF-8
    int len = WideCharToMultiByte(CP_UTF8, 0, pszText, -1, nullptr, 0, nullptr, nullptr);
    std::string result(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, pszText, -1, &result[0], len, nullptr, nullptr);

    GlobalUnlock(hData);
    CloseClipboard();
    return result;
}
```

**After (Platform Abstraction):**
```cpp
#include "core/platform/window_system_interface.h"

std::string getClipboardText(yamy::platform::IWindowSystem* ws) {
    return ws->getClipboardText();  // Returns UTF-8 directly
}
```

## Common Pitfalls and Solutions

### 1. String Encoding

**Pitfall:** Windows uses UTF-16 (wide strings), Linux uses UTF-8.

**Solution:** The platform abstraction always uses UTF-8 (`std::string`). All string conversions happen at the platform boundary.

```cpp
// DON'T: Mix encodings
std::wstring title = L"Hello";  // Wide string
windowSystem->setWindowTitle(hwnd, wstring_to_utf8(title));

// DO: Use UTF-8 consistently
std::string title = "Hello";  // UTF-8 string
windowSystem->setWindowTitle(hwnd, title);
```

### 2. Handle Lifetime

**Pitfall:** Win32 handles may require explicit cleanup (e.g., `CloseHandle()`).

**Solution:** Use RAII with smart pointers or ensure proper cleanup through the abstraction layer.

```cpp
// DON'T: Manual handle management
void* mutex = windowSystem->openMutex("MyMutex");
// ... use mutex
windowSystem->closeHandle(mutex);  // Easy to forget

// DO: RAII wrapper
class MutexGuard {
public:
    MutexGuard(IWindowSystem* ws, const std::string& name)
        : m_ws(ws), m_handle(ws->openMutex(name)) {}
    ~MutexGuard() { if (m_handle) m_ws->closeHandle(m_handle); }
    void* get() const { return m_handle; }
private:
    IWindowSystem* m_ws;
    void* m_handle;
};
```

### 3. Thread Safety

**Pitfall:** Win32 hooks run on dedicated threads; platform abstraction callbacks may run on different threads depending on the platform.

**Solution:** Protect shared state with proper synchronization.

```cpp
#include <mutex>

class SafeKeyHandler {
public:
    bool handleKey(const yamy::platform::KeyEvent& ev) {
        std::lock_guard<std::mutex> lock(m_mutex);
        // Thread-safe access to shared state
        m_lastKeyTime = ev.timestamp;
        return false;
    }
private:
    std::mutex m_mutex;
    uint32_t m_lastKeyTime = 0;
};
```

### 4. Keycode Translation

**Pitfall:** Windows uses virtual key codes (VK_*) and scancodes. Linux uses evdev keycodes.

**Solution:** Use the keycode mapping utilities when working with raw keycodes.

```cpp
#include "platform/linux/keycode_mapping.h"

// Convert Windows scancode to Linux evdev code for comparison
uint16_t winScancode = 0x1E;  // 'A' key scancode
uint16_t evdevCode = yamy::platform::yamyToEvdevKeyCode(winScancode);
```

### 5. Platform-Specific Features

**Pitfall:** Some Win32 features have no Linux equivalent (e.g., MDI windows, UIPI).

**Solution:** Check for feature availability or use alternative approaches.

```cpp
// Feature may not be available on all platforms
bool isMDI = windowSystem->isMDIChild(hwnd);
// On Linux, this always returns false - plan accordingly

// Windows-specific UIPI filter
bool success = windowSystem->changeMessageFilter(msg, MSGFLT_ADD);
// On Linux, this is a no-op - IPC works differently
```

### 6. Error Handling

**Pitfall:** Win32 uses GetLastError(), different platforms have different error codes.

**Solution:** Use exceptions from the platform layer or check return values consistently.

```cpp
#include "core/platform/platform_exception.h"

try {
    auto hook = createInputHook();
    hook->install(callback, nullptr);
} catch (const yamy::platform::PlatformException& e) {
    // Platform-agnostic error handling
    std::cerr << "Hook installation failed: " << e.what() << std::endl;
}
```

### 7. Coordinate Systems

**Pitfall:** Win32 uses top-left origin with Y increasing downward. X11 is the same, but multi-monitor setups may differ.

**Solution:** Use `Rect` consistently and be aware of virtual screen coordinates.

```cpp
// Get primary monitor work area
yamy::platform::Rect workArea;
windowSystem->getWorkArea(&workArea);

// workArea.left may be negative on multi-monitor setups
// Always use workArea bounds, not hardcoded (0,0)
```

## Compilation Errors and Fixes

### Error: `'HWND' was not declared`

**Cause:** Using Win32 types without including `<windows.h>`.

**Fix:** Replace with `WindowHandle`:
```cpp
// Before
HWND hwnd;

// After
yamy::platform::WindowHandle hwnd;
```

### Error: `'GetForegroundWindow' was not declared`

**Cause:** Calling Win32 API directly.

**Fix:** Use interface method:
```cpp
// Before
HWND fg = GetForegroundWindow();

// After
auto fg = windowSystem->getForegroundWindow();
```

### Error: `cannot convert 'RECT*' to 'yamy::platform::Rect*'`

**Cause:** Mixing Win32 and platform types.

**Fix:** Use platform types consistently:
```cpp
// Before
RECT rect;
GetWindowRect(hwnd, &rect);

// After
yamy::platform::Rect rect;
windowSystem->getWindowRect(hwnd, &rect);
```

### Error: `'WH_KEYBOARD_LL' was not declared`

**Cause:** Using Win32 hook constants.

**Fix:** Use `IInputHook` interface instead of raw hook APIs.

### Error: `no matching function for call to 'SendInput'`

**Cause:** Using Win32 input injection.

**Fix:** Use `IInputInjector` interface:
```cpp
// Before
INPUT input = {};
// ... setup input
SendInput(1, &input, sizeof(INPUT));

// After
injector->keyDown(key);
injector->keyUp(key);
```

## Testing Migrated Code

### Unit Testing with Mocks

Create mock implementations for testing without platform dependencies:

```cpp
class MockWindowSystem : public yamy::platform::IWindowSystem {
public:
    WindowHandle getForegroundWindow() override {
        return m_mockForeground;
    }

    std::string getWindowText(WindowHandle hwnd) override {
        return m_windowTitles[hwnd];
    }

    // Setup test data
    void setMockForeground(WindowHandle hwnd) { m_mockForeground = hwnd; }
    void setWindowTitle(WindowHandle hwnd, const std::string& title) {
        m_windowTitles[hwnd] = title;
    }

    // ... implement other methods
private:
    WindowHandle m_mockForeground = nullptr;
    std::map<WindowHandle, std::string> m_windowTitles;
};

TEST(MyComponent, GetsForegroundWindowTitle) {
    MockWindowSystem mockWs;
    mockWs.setMockForeground((void*)0x1234);
    mockWs.setWindowTitle((void*)0x1234, "Test Window");

    MyComponent component(&mockWs);
    EXPECT_EQ("Test Window", component.getActiveTitle());
}
```

### Integration Testing

For integration tests on Linux without a display, use Xvfb:

```bash
# Start virtual framebuffer
Xvfb :99 -screen 0 1024x768x24 &
export DISPLAY=:99

# Run tests
./yamy_tests
```

## Migration Checklist

- [ ] Audit code for Win32 API calls
- [ ] Replace `<windows.h>` includes with platform headers
- [ ] Replace `HWND` with `WindowHandle`
- [ ] Replace `RECT`, `POINT` with `Rect`, `Point`
- [ ] Replace `GetForegroundWindow()` with `IWindowSystem::getForegroundWindow()`
- [ ] Replace `GetWindowText*()` with `IWindowSystem::getWindowText()`
- [ ] Replace `SetWindowsHookEx()` with `IInputHook::install()`
- [ ] Replace `SendInput()` with `IInputInjector` methods
- [ ] Remove manual UTF-16/UTF-8 conversions
- [ ] Add proper thread synchronization for callbacks
- [ ] Inject platform interfaces as dependencies
- [ ] Create unit tests with mock implementations
- [ ] Run integration tests on both Windows and Linux

## Related Documentation

- [Platform Abstraction Architecture](../architecture/platform-abstraction.md) - Full architecture reference
- [Linux Setup Guide](../LINUX-SETUP.md) - Linux development environment setup
- [Building on Linux](../building-linux.md) - Build instructions for Linux
- [Code Quality Guidelines](../code-quality.md) - Coding standards for the project
