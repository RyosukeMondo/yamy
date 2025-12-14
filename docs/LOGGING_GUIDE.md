# YAMY Logging Guide

## Overview

YAMY uses [Quill](https://github.com/odygrd/quill) for zero-latency structured logging. Quill achieves <1μs hot-path overhead through SPSC ring buffers and asynchronous backend threads, making it safe to use even in performance-critical code.

## Quick Reference

```cpp
#include "utils/logger.h"

// Basic logging
LOG_DEBUG("Detailed trace: value={}", value);
LOG_INFO("Informational message: {}", event);
LOG_WARN("Warning: {} exceeded threshold {}", metric, threshold);
LOG_ERROR("Error: failed to load config from {}", path);

// Platform-specific logging (for legacy platform code)
#include "utils/platform_logger.h"
PLATFORM_LOG_INFO("component", "printf-style format %s %d", str, num);
```

## Log Levels

### DEBUG
**When to use**: Detailed diagnostic information useful during development and debugging.

**Examples**:
- Window focus changes
- Key mapping lookups
- State transitions
- Internal function entry/exit (for complex flows)

```cpp
LOG_DEBUG("Window focus changed to: 0x{:x}", window_id);
LOG_DEBUG("Key mapping: evdev {} → yamy 0x{:04X}", evdev_code, yamy_code);
```

**Production behavior**: Disabled in release builds (compile-time optimization).

### INFO
**When to use**: Important informational messages about normal operations.

**Examples**:
- Application startup/shutdown
- Configuration changes
- Successful operations
- Periodic status updates

```cpp
LOG_INFO("YAMY engine initialized successfully");
LOG_INFO("Keyboard layout set to: {}", layout_name);
LOG_INFO("Loaded {} keymaps from config", count);
```

**Production behavior**: Enabled in release builds, written to logs/yamy.json.

### WARN
**When to use**: Potentially problematic situations that aren't errors.

**Examples**:
- Fallback behaviors
- Deprecated functionality usage
- Resource limits approaching
- Retryable failures

```cpp
LOG_WARN("X11 connection not available, using fallback");
LOG_WARN("Config file not found, using defaults: {}", path);
LOG_WARN("Key repeat rate {} exceeds recommended maximum {}", rate, max);
```

**Production behavior**: Enabled in release builds, should trigger monitoring alerts.

### ERROR
**When to use**: Error conditions that prevent normal operation but don't require termination.

**Examples**:
- Failed operations
- Invalid input (external)
- Resource exhaustion
- Unrecoverable component failures

```cpp
LOG_ERROR("Failed to load keymap: {}", error_msg);
LOG_ERROR("X11 error: {}", x11_error_description);
LOG_ERROR("Invalid key code in config: 0x{:04X}", code);
```

**Production behavior**: Enabled in release builds, should trigger alerts and investigation.

## Critical Path Restrictions

**RULE**: Never log inside the input processing loop (hot path).

The input processing loop must complete in <1ms to maintain keyboard responsiveness. Even with Quill's <1μs overhead, logging inside tight loops can accumulate latency.

### DO NOT log in:
- `Engine::processKeyEvent()` main loop
- `KeyMap::lookup()` per-key operations
- Modifier state tracking per-event
- Any function called >1000 times/second

### DO log:
- Initialization and shutdown
- Configuration changes
- State transitions (layer switching, mode changes)
- Errors and warnings
- Periodic statistics (via timer, not per-event)

**Example - BAD**:
```cpp
// ❌ Don't log on every key event
void Engine::processKeyEvent(KeyEvent event) {
    LOG_DEBUG("Processing key event: {}", event.code);  // ❌ Hot path!
    // ... process event ...
}
```

**Example - GOOD**:
```cpp
// ✅ Log on state transitions only
void Engine::switchLayer(int layer_id) {
    LOG_INFO("Layer switched: {} → {}", m_currentLayer, layer_id);  // ✅ Infrequent
    m_currentLayer = layer_id;
}

// ✅ Use metrics for hot-path observability
void Engine::processKeyEvent(KeyEvent event) {
    METRICS_RECORD("input.latency", duration_us);  // ✅ Deferred processing
    // ... process event ...
}
```

## Structured Logging

Quill outputs JSON for AI-compatible log analysis. Use structured fields for queryability.

**Example**:
```cpp
// Each named parameter becomes a JSON field
LOG_INFO("Keymap loaded: name={} keys={} parent={}",
         map_name, key_count, parent_name);
```

**JSON output**:
```json
{
  "timestamp": "2025-12-15 10:30:45.123456789",
  "level": "INFO",
  "message": "Keymap loaded: name=default keys=42 parent=base",
  "logger": "root"
}
```

## Platform Logger (Legacy)

For platform-specific code that needs printf-style formatting:

```cpp
#include "utils/platform_logger.h"

PLATFORM_LOG_DEBUG("component", "format %s", str);
PLATFORM_LOG_INFO("component", "format %d", num);
PLATFORM_LOG_WARN("component", "format %s %d", str, num);
PLATFORM_LOG_ERROR("component", "format %s", error);
```

**Note**: Prefer standard `LOG_*` macros for new code. Platform logger exists for compatibility with legacy C-style formatting.

## Configuration

### Log File Location
- **Path**: `logs/yamy.json`
- **Format**: JSON (one object per line)
- **Rotation**: None (append only)
- **Auto-created**: Yes, `logs/` directory created automatically

### Timestamp Configuration
- **Clock**: RDTSC (CPU timestamp counter)
- **Resync**: Every 250ms (calibrated against system clock)
- **Precision**: Nanosecond-level

### Backend Thread
- **Name**: `yamy-log-backend`
- **Sleep**: 0ns (busy-wait for minimum latency)
- **Flush**: Automatic on shutdown, manual via `yamy::log::flush()`

## Initialization

The logger auto-initializes on first use. No manual initialization required in most cases.

**Explicit initialization** (optional):
```cpp
#include "utils/logger.h"

int main() {
    yamy::log::init();  // Optional: initialize at startup
    // ... application code ...
    yamy::log::flush(); // Optional: ensure all logs written
}
```

## Best Practices

### ✅ DO
- Use structured parameters (`name={} value={}`) instead of concatenation
- Log state transitions and configuration changes
- Use DEBUG for trace-level details
- Keep messages concise and actionable
- Include context (window ID, key code, etc.)

### ❌ DON'T
- Log inside hot loops (>100 calls/second)
- Log sensitive data (passwords, auth tokens)
- Use logging for control flow (don't branch on log level)
- Log every function entry/exit (debug builds only, and sparingly)
- Concatenate strings before logging (`LOG_INFO(str1 + str2)` ❌)

### Format Specifiers
Quill uses libfmt/std::format syntax:

```cpp
LOG_INFO("Hex: 0x{:04X}", value);       // Hex with padding
LOG_INFO("Float: {:.2f}", value);       // 2 decimal places
LOG_INFO("Pointer: {}", (void*)ptr);    // Pointer address
LOG_INFO("Binary: {:08b}", flags);      // 8-bit binary
```

## Debugging

### Viewing Logs
```bash
# View all logs
cat logs/yamy.json

# Pretty-print JSON
jq '.' logs/yamy.json

# Filter by level
jq 'select(.level == "ERROR")' logs/yamy.json

# Search messages
jq 'select(.message | contains("keymap"))' logs/yamy.json
```

### Performance Analysis
```bash
# Count log entries by level
jq -r '.level' logs/yamy.json | sort | uniq -c

# Find slow operations (if timing fields present)
jq 'select(.duration_us > 1000)' logs/yamy.json
```

## Common Pitfalls

### 1. Logging in Hot Path
```cpp
// ❌ BAD: Logs every key press
void processKey(int code) {
    LOG_DEBUG("Key: {}", code);  // Called 1000+ times/second!
}

// ✅ GOOD: Log only exceptional cases
void processKey(int code) {
    if (code > MAX_KEYCODE) {
        LOG_ERROR("Invalid key code: {}", code);  // Rare error case
    }
}
```

### 2. String Concatenation
```cpp
// ❌ BAD: Pre-concatenates string
LOG_INFO(std::string("Message: ") + value);

// ✅ GOOD: Let Quill format
LOG_INFO("Message: {}", value);
```

### 3. Expensive Computations
```cpp
// ❌ BAD: Computation happens even if DEBUG disabled
LOG_DEBUG("State: {}", computeExpensiveDebugString());

// ✅ GOOD: Guard expensive computations
#ifndef NDEBUG
    LOG_DEBUG("State: {}", computeExpensiveDebugString());
#endif
```

## Support

For Quill-specific documentation, see: https://github.com/odygrd/quill

For YAMY logging issues:
1. Check `logs/yamy.json` for errors during initialization
2. Verify `logs/` directory is writable
3. Ensure Quill dependency installed via Conan (`quill/4.1.0`)
