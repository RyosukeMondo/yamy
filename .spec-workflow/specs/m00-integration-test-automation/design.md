# Design: M00 Integration Test Automation

## Executive Summary

The root cause of integration test failures is **NOT "mock environment limitations"** - it's a combination of:
1. **Race conditions** in test setup (Engine not fully initialized before events injected)
2. **Missing evdev code mapping** (tests use YAMY scan codes, Engine expects evdev codes)
3. **Async event processing** without proper synchronization
4. **Insufficient test infrastructure** for time-based hold/tap detection

This design fixes the real issues and creates fully automated verification.

## Architecture Analysis

### Current State (Broken)

```
Test → injectKey(0x1e) → MockInputHook.callback() → Engine.pushInputEvent()
                                                           ↓ (async queue)
                                                    KeyboardHandler thread
                                                           ↓
                                                    EventProcessor.processEvent()
                                                           ↓
                                                    ❌ PROBLEM: Events processed but no output!
```

**Why it fails:**
1. **Timing Issue**: Test injects events before Engine is fully initialized
2. **Code Mismatch**: Test uses YAMY codes (0x1e), but KeyEvent.scanCode should be evdev code (30)
3. **No Synchronization**: Test doesn't wait for async processing to complete
4. **Missing Activation**: M00 modifier not activating because hold threshold checking requires time passage

### Target State (Working)

```
Test → EventSimulator.inject(evdev=30, delay=250ms) →
                                                      ↓
                                               Proper evdev codes
                                                      ↓
                                          Wait for initialization
                                                      ↓
                                             Engine processes
                                                      ↓
                                          Wait for async queue
                                                      ↓
                                     MockInjector receives output ✅
```

## Component Design

### 1. EventSimulator (New)

**Purpose**: Inject events with proper timing and synchronization

```cpp
class EventSimulator {
public:
    struct Event {
        uint16_t evdev_code;
        bool is_key_down;
        uint32_t delay_before_ms;
    };

    // Inject sequence of events with timing
    void injectSequence(Engine* engine, const std::vector<Event>& events);

    // Wait for engine to be ready
    bool waitForEngineReady(Engine* engine, uint32_t timeout_ms = 5000);

    // Wait for output to appear
    bool waitForOutput(MockInputInjector* injector, uint32_t timeout_ms = 1000);

private:
    void sleepMs(uint32_t ms);
    std::chrono::steady_clock::time_point m_start_time;
};
```

**Key Features:**
- **Proper delays**: Wait between events to simulate real timing
- **Synchronization**: Wait for engine initialization and async processing
- **Evdev codes**: Use correct evdev codes, not YAMY scan codes

### 2. EngineTestFixture (Enhanced)

**Purpose**: Provide fully initialized Engine with proper mocks

```cpp
class EngineTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mocks
        mockInjector = new MockInputInjector();
        mockHook = new MockInputHook();
        mockDriver = new MockInputDriver();

        // Create engine
        engine = new Engine(...);

        // Load config
        loadConfig(configJson);

        // Start engine
        engine->start();

        // CRITICAL: Wait for full initialization
        ASSERT_TRUE(simulator.waitForEngineReady(engine, 5000))
            << "Engine failed to initialize in 5 seconds";

        // CRITICAL: Wait for setting to be applied
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    void loadConfig(const std::string& jsonContent);

    EventSimulator simulator;
    Engine* engine;
    MockInputInjector* mockInjector;
    MockInputHook* mockHook;
    MockInputDriver* mockDriver;
};
```

**Key Improvements:**
- **Proper initialization wait**: Don't inject events until Engine is ready
- **Setting application wait**: Give EventProcessor time to register M00 triggers
- **Clean lifecycle**: Proper setup/teardown

### 3. Test Scenarios (Declarative)

**Purpose**: Define test cases as event sequences

```cpp
// Test: Tap A <200ms → outputs B
TEST_F(EngineTestFixture, TapAShouldOutputB) {
    loadConfig(TEST_CONFIG_M00);

    // Define event sequence
    std::vector<EventSimulator::Event> events = {
        {30, true, 0},      // Press A (evdev 30)
        {30, false, 100}    // Release A after 100ms (< 200ms threshold)
    };

    mockInjector->reset();
    simulator.injectSequence(engine, events);

    // Wait for processing
    ASSERT_TRUE(simulator.waitForOutput(mockInjector, 1000));

    // Verify tap output
    EXPECT_EQ(mockInjector->lastMakeCode, 0x30) << "Should output B";
    EXPECT_EQ(mockInjector->injectCallCount, 2) << "Should output press + release";
}

// Test: Hold A >200ms → A suppressed, M00 active
TEST_F(EngineTestFixture, HoldAShouldSuppressA) {
    loadConfig(TEST_CONFIG_M00);

    std::vector<EventSimulator::Event> events = {
        {30, true, 0},      // Press A
        {31, true, 250},    // Press S after 250ms (A held >200ms)
        {31, false, 50},    // Release S
        {30, false, 50}     // Release A
    };

    mockInjector->reset();
    simulator.injectSequence(engine, events);

    ASSERT_TRUE(simulator.waitForOutput(mockInjector, 1000));

    // Verify M00+S outputs D (not S)
    EXPECT_EQ(mockInjector->lastMakeCode, 0x20) << "M00+S should output D";
    EXPECT_EQ(mockInjector->injectCallCount, 2) << "Only D press+release, A suppressed";
}
```

### 4. Enhanced Logging

**Purpose**: Debug test failures with detailed logs

Add to EventProcessor and ModifierKeyHandler:

```cpp
// In EventProcessor::processEvent()
LOG_DEBUG("[TEST] processEvent: evdev={}, type={}", input_evdev, (int)type);

if (m_modifierHandler && io_modState) {
    auto to_activate = m_modifierHandler->checkAndActivateWaitingModifiers();
    for (const auto& [scancode, mod_num] : to_activate) {
        LOG_DEBUG("[TEST] Activating M{:02X} (trigger key: 0x{:04X})",
                  mod_num, scancode);
        io_modState->activateModifier(mod_num);
    }
}

LOG_DEBUG("[TEST] processEvent result: output_evdev={}, output_yamy={}, valid={}, is_tap={}",
          result.output_evdev, result.output_yamy, result.valid, result.is_tap);
```

## Data Flow

### Correct Event Flow

```
1. Test Setup Phase
   EventSimulator.waitForEngineReady()
   ↓
   Engine::start() completes
   ↓
   Engine::setSetting() completes
   ↓
   EventProcessor created
   ↓
   ModifierKeyHandler initialized
   ↓
   M00 trigger registered: A (0x1E) → M00 with tap=B (0x30)
   ↓
   ✅ Engine READY

2. Event Injection Phase
   Test: simulator.injectSequence([Press A, Wait 250ms, Release A])
   ↓
   EventSimulator converts to KeyEvent with EVDEV code (30, not 0x1E)
   ↓
   mockHook.callback(KeyEvent{scanCode=30, isKeyDown=true})
   ↓
   Engine.pushInputEvent() → queue
   ↓
   KeyboardHandler thread wakes up

3. Processing Phase
   KeyboardHandler reads from queue
   ↓
   Converts KeyEvent to KEYBOARD_INPUT_DATA
   ↓
   c.m_evdev_code = 30
   ↓
   beginGeneratingKeyboardEvents()
   ↓
   EventProcessor.processEvent(evdev=30, type=PRESS, modState)
   ↓
   checkAndActivateWaitingModifiers() → M00 state=WAITING
   ↓
   [Wait 250ms - in simulator]
   ↓
   Next event arrives
   ↓
   checkAndActivateWaitingModifiers() → threshold exceeded → M00 ACTIVE
   ↓
   layer2_applySubstitution() with M00 active
   ↓
   Rule lookup: M00+S → D
   ↓
   generateKeyEvent(D)
   ↓
   injectInput(&kid)
   ↓
   mockInjector->inject() ✅
```

## Key Design Decisions

### Decision 1: Use Evdev Codes in Tests

**Problem**: Tests were using YAMY scan codes (0x1E) directly
**Solution**: Convert to evdev codes (30) before injection
**Rationale**: Engine's KeyEvent.scanCode is actually evdev code, despite the name

### Decision 2: Explicit Timing Control

**Problem**: No control over event timing, race conditions
**Solution**: EventSimulator with `delay_before_ms` per event
**Rationale**: Hold/tap detection requires precise timing (200ms threshold)

### Decision 3: Synchronization Points

**Problem**: Tests inject events before Engine is ready
**Solution**: `waitForEngineReady()` and `waitForOutput()` helpers
**Rationale**: Async initialization and processing need explicit waits

### Decision 4: Re-enable All Tests

**Problem**: Tests were disabled claiming "mock limitations"
**Solution**: Fix the real issues, re-enable tests
**Rationale**: Mock environment CAN work - we just need to use it correctly

## Implementation Strategy

### Phase 1: Foundation (Tasks 1-2)
- Create EventSimulator utility
- Add synchronization helpers
- Add test logging

### Phase 2: Fix Test Infrastructure (Tasks 3-4)
- Update EngineTestFixture with proper initialization
- Fix evdev code mapping
- Add timing control

### Phase 3: Re-enable Tests (Task 5)
- Update all 5 test cases with correct approach
- Remove DISABLED_ prefix
- Verify all pass

### Phase 4: Verification (Task 6)
- Run tests 100 times to verify reliability
- Add to CI/CD pipeline
- Document testing approach

## Testing Strategy

### Unit Tests (Existing - Already Passing)
- EventProcessor logic ✅
- ModifierKeyHandler state machine ✅
- Rule lookup ✅

### Integration Tests (This Spec - To Fix)
- Full Engine lifecycle with M00
- Tap vs hold detection
- Rule application with active modifiers
- Output generation

### Acceptance Criteria
- All 5 integration tests pass consistently
- Tests run in <10 seconds
- No manual verification needed
- Can run in CI/CD

## Risks & Mitigation

| Risk | Mitigation |
|------|------------|
| Thread timing still flaky | Add retry logic with exponential backoff |
| Engine initialization varies | Increase wait timeouts, add ready check |
| Evdev mapping incomplete | Create comprehensive key code lookup table |
| Tests still fail | Add detailed logging to diagnose exact failure point |

## Success Metrics

1. **All 5 tests passing** - No DISABLED_ prefix
2. **100% reliability** - Pass 100 consecutive runs
3. **Fast execution** - <10 seconds total
4. **Clear failures** - Detailed error messages when tests fail
5. **Zero manual testing** - Commit with confidence

## References

- Existing unit tests: `tests/test_m00_virtual_modifier_ut.cpp` (working correctly)
- Broken integration tests: `tests/test_m00_integration.cpp` (to be fixed)
- EventProcessor: `src/core/engine/engine_event_processor.cpp`
- ModifierKeyHandler: `src/core/engine/modifier_key_handler.cpp`
