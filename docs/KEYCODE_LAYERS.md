## YAMY Keycode Transformation Layers

Complete guide to understanding how keycodes flow through YAMY's transformation pipeline.

## Overview

When you press a key, it goes through multiple transformation layers:

```
┌──────────────┐
│ Physical Key │  User presses 'A' on keyboard
└──────┬───────┘
       │
       ▼
┌──────────────────────────────────────────────────────────┐
│ LAYER 1: Input (Linux evdev → YAMY scan code)          │
│                                                          │
│ Function: evdevToYamyKeyCode(evdev_code)                │
│ Location: src/platform/linux/keycode_mapping.cpp        │
│ Maps: Linux KEY_A (evdev 30) → YAMY VK_A (0x41)        │
└──────┬───────────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────────────────┐
│ LAYER 2: Engine Processing (def subst transformations)  │
│                                                          │
│ Function: engine.cpp::keyboardHandler()                  │
│ Location: src/core/engine/engine.cpp                    │
│ Applies: config.mayu "def subst" rules                  │
│ Example: def subst *A = *Tab  (A → Tab)                 │
└──────┬───────────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────────────────┐
│ LAYER 3: Output (YAMY scan code → Linux evdev)         │
│                                                          │
│ Function: yamyToEvdevKeyCode(yamy_code)                 │
│ Location: src/platform/linux/keycode_mapping.cpp        │
│ Maps: YAMY VK_TAB (0x09) → Linux KEY_TAB (evdev 15)    │
│ Layout-aware: Uses US or JP scan code map              │
└──────┬───────────────────────────────────────────────────┘
       │
       ▼
┌──────────────┐
│ Output Event │  System receives 'Tab' keypress
└──────────────┘
```

## Layer 1: Input Translation

**File**: `src/platform/linux/keycode_mapping.cpp`
**Function**: `evdevToYamyKeyCode()`
**Purpose**: Convert Linux evdev codes to YAMY's internal codes

### Mapping Table

Uses `g_evdevToYamyMap` (bidirectional with `g_yamyToEvdevMap`):

```cpp
// Example mappings:
KEY_A (30) → VK_A (0x41)
KEY_TAB (15) → VK_TAB (0x09)
KEY_ENTER (28) → VK_RETURN (0x0D)
```

### Debug Logging

Enable with `YAMY_DEBUG_KEYCODE=1`:

```
[LAYER1:IN] Input evdev code = 30 (KEY_A)
[LAYER1:IN] Mapped to YAMY code = 0x0041 (65)
```

## Layer 2: Engine Processing

**File**: `src/core/engine/engine.cpp`
**Function**: `keyboardHandler()`
**Purpose**: Apply configuration transformations (def subst, mod, key mappings)

### Transformations Applied

1. **Simple Substitution** (`def subst`):
   ```
   def subst *A = *Tab    # A → Tab
   ```

2. **Modal Layers** (`mod` + `key`):
   ```
   mod mod0 = !!B         # Hold B to activate mod0
   key m0-*A = *Enter     # When mod0 active, A → Enter
   ```

3. **Modifier Redefinitions**:
   ```
   mod Ctrl += LAlt       # LAlt becomes Ctrl
   ```

### Debug Logging

Currently limited. To add:
```cpp
// In engine.cpp::keyboardHandler()
PLATFORM_LOG_INFO("engine", "[LAYER2:ENGINE] Input scan=0x%04X After subst=0x%04X",
                  originalScan, transformedScan);
```

## Layer 3: Output Translation

**File**: `src/platform/linux/keycode_mapping.cpp`
**Function**: `yamyToEvdevKeyCode()`
**Purpose**: Convert YAMY codes back to Linux evdev codes

### Layout-Aware Mapping

The function checks keyboard layout and uses appropriate map:

```cpp
std::string layout = detectKeyboardLayout();  // Returns "us" or "jp"

if (layout == "jp") {
    // Use g_scanToEvdevMap_JP
} else {
    // Use g_scanToEvdevMap_US (default)
}
```

### Mapping Priority

1. **Try VK code map first** (`g_yamyToEvdevMap`):
   ```cpp
   VK_A (0x41) → KEY_A (30)
   VK_TAB (0x09) → KEY_TAB (15)
   ```

2. **If not found, try scan code map** (layout-specific):
   ```cpp
   // US layout:
   0x1E → KEY_A (30)

   // JP layout:
   0x1E → KEY_A (30)
   0x7D → KEY_YEN (124)  // JP-specific
   ```

### Debug Logging

Enable with `YAMY_DEBUG_KEYCODE=1`:

```
[LAYER3:OUT] Input YAMY code = 0x0009 (9)
[LAYER3:OUT] Found in VK map → evdev 15 (KEY_TAB)
```

or

```
[LAYER3:OUT] Input YAMY code = 0x001E (30)
[LAYER3:OUT] Not in VK map, using scan map for layout: jp
[LAYER3:OUT] Found in JP scan map → evdev 30 (KEY_A)
```

## Layout Specification

### Auto-Detection (Default)

YAMY automatically detects layout using `setxkbmap`:

```cpp
std::string detectKeyboardLayout() {
    // Runs: setxkbmap -query | grep 'layout:' | awk '{print $2}'
    // Returns: "us", "jp", etc.
}
```

### Manual Override (Config File)

**Not yet implemented** - Planned feature:

```cpp
// In config parsing:
if (line == "# LAYOUT: US") {
    yamy::platform::setLayoutOverride("us");
}
```

### Temporary Override (Code)

```cpp
#include "platform/linux/keycode_mapping.h"

// Force US layout
yamy::platform::setLayoutOverride("us");

// Clear override (use auto-detection)
yamy::platform::clearLayoutOverride();
```

## Investigation Tools

### 1. Layer Investigation Script

**File**: `tests/investigate_keycode_layers.sh`

```bash
# Investigate what happens to key 'A' with config.mayu
./tests/investigate_keycode_layers.sh config.mayu 30 A

# Output shows all 3 layers:
# LAYER 1: evdev 30 → YAMY 0x0041
# LAYER 2: (engine processing)
# LAYER 3: YAMY 0x0009 → evdev 15 (if A→Tab substitution)
```

### 2. E2E Testing

**File**: `tests/run_comprehensive_e2e_tests.sh`

```bash
# Run all E2E tests
./tests/run_comprehensive_e2e_tests.sh

# Tests verify end-to-end behavior:
# Input → YAMY → Output
```

### 3. Manual Debug Logging

```bash
# Enable debug logging
export YAMY_DEBUG_KEYCODE=1

# Start YAMY
./build/bin/yamy

# Check logs
tail -f /tmp/yamy_debug.log | grep LAYER
```

## Common Issues & Debugging

### Issue 1: Wrong Output for Substitution

**Symptom**: `def subst *A = *Tab` produces wrong key

**Debug Steps**:
1. Enable debug logging: `YAMY_DEBUG_KEYCODE=1`
2. Inject test key: `yamy-test e2e-auto 30 15`
3. Check logs:
   ```
   [LAYER1:IN] Input evdev code = 30 (KEY_A)
   [LAYER1:IN] Mapped to YAMY code = 0x0041 (65)
   [LAYER3:OUT] Input YAMY code = 0x0009 (9)   # Should be VK_TAB
   [LAYER3:OUT] Found in VK map → evdev 15 (KEY_TAB)
   ```

4. **If LAYER3 input is wrong**: Problem in Layer 2 (engine processing)
5. **If LAYER3 output is wrong**: Problem in yamyToEvdevKeyCode mapping

### Issue 2: Layout Mismatch

**Symptom**: Keys work on US layout but not JP layout (or vice versa)

**Debug Steps**:
1. Check detected layout:
   ```bash
   setxkbmap -query | grep layout
   ```

2. Check YAMY's detection:
   ```bash
   grep "Detected keyboard layout" /tmp/yamy_debug.log
   ```

3. **If mismatch**: Use layout override:
   ```cpp
   yamy::platform::setLayoutOverride("jp");  // or "us"
   ```

### Issue 3: Scan Code Not in Map

**Symptom**: Debug log shows "NOT FOUND in US/JP scan map"

**Solution**: Add missing scan code to mapping table:

```cpp
// In keycode_mapping.cpp, find g_scanToEvdevMap_US or g_scanToEvdevMap_JP

const std::unordered_map<uint16_t, uint16_t> g_scanToEvdevMap_US = {
    // ... existing mappings ...
    {0xYOUR_SCAN_CODE, KEY_YOUR_KEY},  // Add this
};
```

## Testing Scan Code Mappings

### Unit Test Template

```cpp
// Test: Verify scan code mapping for US layout
yamy::platform::setLayoutOverride("us");
uint16_t result = yamy::platform::yamyToEvdevKeyCode(0x1E);
ASSERT_EQ(result, KEY_A);  // Should map to KEY_A

// Test: Verify scan code mapping for JP layout
yamy::platform::setLayoutOverride("jp");
result = yamy::platform::yamyToEvdevKeyCode(0x7D);
ASSERT_EQ(result, KEY_YEN);  // JP-specific key
```

### E2E Test Template

```bash
# Test substitution with US layout
echo '# LAYOUT: US' > test_us.mayu
echo 'include "config.mayu"' >> test_us.mayu

# Run test
yamy-test e2e-auto 30 15  # A → Tab
# Expected: PASS

# Test substitution with JP layout
echo '# LAYOUT: JP' > test_jp.mayu
echo 'include "config.mayu"' >> test_jp.mayu

# Run test
yamy-test e2e-auto 30 15  # A → Tab
# Expected: PASS (should work on both layouts)
```

## Next Steps

1. **Implement config-based layout override**
   - Parse `# LAYOUT: US` comments in .mayu files
   - Call `setLayoutOverride()` from config parser

2. **Add Layer 2 debug logging**
   - Log before/after def subst transformations
   - Show modal layer activations

3. **Create comprehensive unit tests**
   - Test all VK code mappings
   - Test US vs JP scan code mappings
   - Test layout override functionality

4. **Build scan code discovery tool**
   - Inject evdev codes and capture YAMY codes
   - Build complete mapping table automatically
