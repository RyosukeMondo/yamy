# YAMY Linux Testing Strategy

## Overview
Minimize UAT by creating automated testing infrastructure for keyboard remapping validation.

## 1. Virtual Key Injection CLI Tool

### yamy-test CLI Command
```bash
# Single key injection
yamy-test inject --key a --layout jp --expect "abc"

# Key sequence
yamy-test sequence --keys "abc" --layout jp --config master.mayu

# Dry-run mode (no actual injection, just log what would happen)
yamy-test dry-run --keys "abc" --layout jp

# Verify mapping
yamy-test verify --input "a" --output "{" --config dvorakY.mayu --layout us
```

### Features
- Simulate keyboard events via uinput
- Capture injected output
- Compare expected vs actual
- Support different layouts (us, jp)
- Support different configs (.mayu files)

## 2. Test Cases Structure

### Test Definition Format (YAML)
```yaml
# tests/keymap/dvorak_us.yaml
name: "DvorakY US Layout Basic"
layout: us
config: dvorakY.mayu
tests:
  - input: "a"
    expected: "{"
    description: "a remaps to Shift+["

  - input: "abc"
    expected: "{bc"
    description: "Multiple key sequence"
```

### Test Categories
- **Unit Tests**: Individual components
  - Layout detection
  - Keycode mapping (scan → evdev)
  - Configuration parsing

- **Integration Tests**: Full pipeline
  - Input hook → Queue → Handler → Injection
  - Different layouts (us, jp)
  - Different configs

- **Regression Tests**: Known issues
  - Key accumulation bug (fixed)
  - Event signaling bug (fixed)
  - Mouse grab issue (fixed)

## 3. Implementation Plan

### Phase 1: Virtual Key Injector (Week 1)
```cpp
// src/test/virtual_keyboard.cpp
class VirtualKeyboard {
public:
    void sendKey(uint16_t evdev_code, bool press);
    void sendSequence(const std::string& keys);
    std::string captureOutput();
};
```

### Phase 2: Test Runner (Week 1)
```cpp
// src/test/test_runner.cpp
class TestRunner {
public:
    bool runTest(const TestCase& test);
    TestReport runSuite(const std::string& suite_file);
};
```

### Phase 3: Automated CI (Week 2)
```yaml
# .github/workflows/test.yml
- name: Run keyboard mapping tests
  run: |
    ./build/bin/yamy-test suite tests/keymap/*.yaml
```

## 4. Coverage Goals

- **Unit Tests**: 80% code coverage
  - keycode_mapping.cpp: 100%
  - input_injector_linux.cpp: 90%
  - sync_linux.cpp: 90%

- **Integration Tests**: Key scenarios
  - US layout + dvorakY.mayu
  - JP layout + 109.mayu only
  - JP layout + config.mayu
  - Layout switching (us ↔ jp)

- **UAT**: Minimize to <5%
  - Only new features
  - Edge cases not covered by automated tests

## 5. Dry-Run Mode

### Example Output
```bash
$ yamy-test dry-run --keys "abc" --layout jp --config master.mayu

[DRY-RUN] Layout: jp
[DRY-RUN] Config: master.mayu (109.mayu + hm.mayu)
[DRY-RUN]
[DRY-RUN] Input 'a' (evdev=30):
[DRY-RUN]   → Captured by input hook
[DRY-RUN]   → Converted to YAMY code: 0x1E
[DRY-RUN]   → Remapped by config to: <no remap>
[DRY-RUN]   → Output scan code: 0x1E
[DRY-RUN]   → Converted to evdev: 30
[DRY-RUN]   → Injected: KEY_A (press)
[DRY-RUN]   → Expected output: 'a'
[DRY-RUN]
[DRY-RUN] ✓ All 3 keys processed successfully
```

## 6. Benefits

### For Development
- Fast feedback loop (seconds vs minutes)
- Reproducible test cases
- Regression prevention
- CI/CD integration

### For Users
- Confidence in releases
- Clear test coverage
- Known limitations documented
- Self-service validation

## 7. Tools to Build

1. **yamy-test** - Main test CLI
2. **yamy-inject** - Low-level key injection
3. **yamy-capture** - Output capture utility
4. **yamy-verify** - Mapping verification

## Next Steps

1. Create yamy-test tool skeleton
2. Implement virtual keyboard class
3. Add basic test cases
4. Integrate with CI
5. Document test writing guide
