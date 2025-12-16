# Test Infrastructure Refactoring Report

**Status:** ✅ Refactoring Complete & Verified

## Problem
The previous E2E test infrastructure (`yamy-test` binary + shell scripts) was flaky and unreliable because:
1.  **Device Churn:** `yamy-test inject` created and destroyed a `uinput` device for *every single key event*. This overwhelmed YAMY's device detection and caused missed events.
2.  **Restart Loop:** `e2e-auto` mode restarted the YAMY daemon for every test case, making the suite incredibly slow and prone to race conditions.
3.  **Log Parsing:** Output verification relied on parsing debug logs, which is fragile and asynchronous.
4.  **Timing Control:** It was difficult to precisely test Tap vs Hold timing.

## Solution: New Hybrid Architecture

We implemented a robust hybrid testing framework:

### 1. Enhanced `yamy-test` (C++)
- Added `interactive` mode.
- Creates a single persistent virtual keyboard.
- Listens on `stdin` for commands: `PRESS <code >`, `RELEASE <code >`, `WAIT <ms>`.
- Allows precise timing control from the test controller.

### 2. Refactored `automated_keymap_test.py` (Python)
- Acts as the test controller.
- **Persistent Environment:** Starts `yamy-test interactive` and `yamy` daemon *once* at the beginning.
- **Direct Event Capture:** Uses `python-evdev` to read directly from YAMY's virtual output device (`/dev/input/eventX`), bypassing logs entirely for verification.
- **Smart Synchronization:** Drains event queues and synchronizes input/output streams.
- **Logic Support:** Correctly handles Tap (output on release) vs Normal (output on press) verification logic.

## Verification
The new framework successfully verified the M00-MFF migration:
- **TAP Test:** `B` -> `Enter` ✅ (Verified correct Tap behavior)
- **HOLD Test:** `B` (hold) + `W` -> `LShift` ✅ (Verified correct Modifier behavior)

## Usage
```bash
python3 tests/automated_keymap_test.py --config keymaps/master.mayu
```

## Next Steps
- Port the full suite of test cases from `e2e_test_cases.txt` to use this new Python driver.
- Deprecate the old shell-based runner.
