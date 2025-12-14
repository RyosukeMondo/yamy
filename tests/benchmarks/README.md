# Performance Benchmarks for Investigate Window Feature

This directory contains performance benchmarks designed to validate that all latency and throughput requirements are met for the investigate window feature.

## Benchmark Tests

### 1. Window Property Query Latency
**Target:** <10ms (P99)
**Measures:** Combined time for all window property queries:
- `getWindowText()`
- `getClassName()`
- `getWindowProcessId()`
- `getWindowRect()`
- `getShowCommand()`

### 2. IPC Round-Trip Latency
**Target:** <5ms (P99)
**Simulates:** Dialog sends `CmdInvestigateWindow` → Engine responds with `RspInvestigateWindow`

### 3. End-to-End Investigate Latency
**Target:** <10ms (P99)
**Combines:** windowFromPoint + property queries + IPC round-trip
**Simulates:** Complete workflow from window selection to all panels populated

### 4. windowFromPoint Performance
**Target:** <2ms (P99)
**Measures:** Cursor-to-window lookup time for crosshair widget

### 5. Stress Test: Rapid Key Events
**Target:** 50 keys/sec with 0 dropped events and <5% CPU
**Simulates:** Live key event logging under high load

## Status

**Design Complete:** All benchmark tests have been designed and implemented in `investigate_performance_test.cpp`.

**Build Integration:** Pending - The test has been added to CMakeLists.txt but requires resolving X11/Qt header conflicts and proper include paths.

## Build Issues Encountered

1. **X11/Qt Header Conflicts:** X11 headers define `Bool` and `Status` macros that conflict with Qt types. Solution: Include X11 headers after Qt headers.

2. **Include Path Resolution:** The test needs proper include path configuration to find WindowSystemLinux headers.

3. **Type Conversions:** `Window` (X11 type, unsigned long) needs casting to `WindowHandle` (void*).

## Running the Benchmarks

Once build issues are resolved:

```bash
cmake --build build --target yamy_investigate_performance_test
./build/bin/yamy_investigate_performance_test
```

## Expected Output

The benchmarks print detailed statistics including:
- Sample count
- Min/Max latency
- P50, P95, P99 percentiles
- Average latency

All assertions verify that performance targets are met.

## Future Work

1. Resolve build configuration to compile the test
2. Run benchmarks on target hardware
3. Profile and optimize if targets aren't met
4. Add CI integration to track performance over time
5. Add memory usage benchmarks (via Valgrind)
