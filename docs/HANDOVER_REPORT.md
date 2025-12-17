# Handover Report: Core Refactoring & E2E Test Infrastructure

**Date:** 2025-12-17
**Status:** Core Refactoring Complete | E2E Framework Complete | **E2E Runtime Fails**

## 1. Accomplishments

### Core Engine Refactoring
We successfully modernized the legacy core engine, moving from a fragile linear-scan architecture to a robust compiled-lookup model.
*   **AST-Based Parsing:** `SettingLoader` now populates a `ConfigAST` (`src/core/settings/config_ast.h`), decoupling parsing from runtime object creation.
*   **Unified Modifier State:** Replaced `Modifier` (legacy) and `LockState` with a unified `yamy::input::ModifierState` using `std::bitset` to support 256 virtual modifiers (`M00`-`MFF`) and locks.
*   **Optimized Event Lookup:** Implemented `RuleLookupTable` and `CompiledRule`. `EventProcessor` now performs O(1) lookups instead of O(N) scans.
*   **Cleanup:** Removed `thread_local` hacks and legacy `m_substitutionTable`.

### E2E Test Infrastructure
We replaced the monolithic `automated_keymap_test.py` with a modular framework in `tests/framework/`:
*   **Components:** `ConfigParser`, `EventInjector` (wraps `yamy-test`), `LogMonitor` (evdev capture), `ReportGenerator`.
*   **Runner:** `tests/e2e/test_runner.py` supports `--quick` mode and robust process lifecycle management.
*   **Fixes:** Resolved self-termination bugs (`pkill` pattern) and device enumeration skipping issues.

## 2. Current Status & Limitations

### Build & Run
*   **Build:** Successful via Ninja. `ninja -C build_ninja yamy yamy-ctl yamy-test`.
*   **Execution:** `yamy` daemon starts correctly, loads `keymaps/config_clean.mayu`, and registers virtual modifiers.
*   **Injection:** `yamy-test` successfully injects events into a virtual input device. `yamy` successfully reads them (confirmed via logs).

### The Failure: "No output event received"
Running `python3 tests/e2e/test_runner.py ...` results in all tests failing.
*   **Symptoms:** `LogMonitor` attaches to `Yamy Remapped Output Device` but captures **zero events**.
*   **Evidence:**
    *   `yamy` logs show correct processing: `[OUTPUT] Injecting evdev code 0xf (TAB) DOWN`.
    *   This proves `yamy` IS writing to the uinput file descriptor.
    *   `LogMonitor` (using `python-evdev`) sees no data on the corresponding `/dev/input/eventX` node.

### Potential Root Causes
1.  **Environment/UInput:** The container/VM environment might have issues propagating `uinput` writes back to the read interface efficiently, or permission issues despite `sudo/input` group (though `evdev.InputDevice` creation succeeds).
2.  **Timing/Select:** The Python `select.select` call in `LogMonitor` might be timing out or behaving unexpectedly.
3.  **Device Grabbing:** Although we tried disabling `grab()`, conflicting grabs between the kernel, X11/Wayland (if present), or `yamy` itself could be an issue.

## 3. Known Bugs & Fixes Applied

### `Delete` Scan Code Mismatch
*   **Issue:** `C -> Delete` test failed with "Expected 111, Got 83".
*   **Analysis:** `SettingLoader::load_SCAN_CODES` was parsing `E0-0x53` as just `0x53` (KPDot) instead of merging the `E0` flag to make `0xE053` (Delete).
*   **Fix Applied:** Updated `setting_loader.cpp` to `sc.m_scan |= 0xE000` when `E0` flag is present.
*   **Verification:** Pending (requires working Monitor).

### Legacy Config Support
*   **Issue:** `config_clean.mayu` used `mod mod0 = !!B` which failed with the new engine.
*   **Fix Applied:** Updated `config_clean.mayu` to use `def numbermod *B = *M00` and `key M00-A = ...`.
*   **Result:** Config loads successfully.

## 4. Plan to "Pass Every Test"

To achieve the goal of passing all tests, the following steps are required:

### Step 1: Fix LogMonitor (Critical)
Isolate the `uinput` read issue.
*   **Action:** Create a minimal reproduction script (`debug_uinput.py`) that creates a uinput device, writes a key, and tries to read it back immediately in the same or separate process.
*   **Action:** If `python-evdev` is flaky, try reading `/dev/input/eventX` raw bytes in Python.
*   **Action:** Verify if `yamy`'s `InputInjectorLinux` needs to set specific properties (e.g., `EV_REP`) to be fully recognized by the kernel as a "read-back" device.

### Step 2: Verify Keycode Fixes
Once monitoring works, confirm that `C -> Delete` output is now `111` (Delete) and not `83` (KPDot).

### Step 3: Handle Passthrough Events
Many tests failed because `yamy` suppresses events it doesn't remap (or `LogMonitor` missed them).
*   **Action:** Verify if `EventProcessor` passes through non-mapped keys (`layer2` returns input -> `layer3` outputs it).
*   **Action:** If passthrough is working but tests fail, ensure `LogMonitor` isn't filtering them.

### Step 4: Expand Test Coverage
*   **Action:** Add tests for the new M00-MFF modifiers specifically.
*   **Action:** Restore the full test suite (remove `--quick`) and run.

## 5. Quick Commands

**Build:**
```bash
cmake -B build_ninja -G Ninja -Dfmt_DIR=$(pwd)/build_ninja -Dquill_DIR=$(pwd)/build_ninja -DMicrosoft.GSL_DIR=$(pwd)/build_ninja -DCMAKE_TOOLCHAIN_FILE=build_ninja/conan_toolchain.cmake -DCMAKE_PREFIX_PATH=$(pwd)/build_ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_LINUX_STUB=ON -DBUILD_QT_GUI=ON
ninja -C build_ninja yamy yamy-ctl yamy-test
```

**Run E2E Tests:**
```bash
python3 tests/e2e/test_runner.py \
    --config keymaps/config_clean.mayu \
    --yamy-path build_ninja/bin/yamy \
    --yamy-ctl-path build_ninja/bin/yamy-ctl \
    --yamy-test-path build_ninja/bin/yamy-test \
    --quick
```
