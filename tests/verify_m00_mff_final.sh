#!/bin/bash
# Phase 3 Verification Script - M00-MFF System

set -e

# Cleanup previous runs
pkill -9 yamy 2>/dev/null || true
rm -f /tmp/yamy_phase3.log

echo "=== Phase 3 Verification: M00-MFF System ==="

# 1. Start Daemon
echo "1. Starting yamy daemon..."
./build/bin/yamy > /tmp/yamy_phase3.log 2>&1 &
DAEMON_PID=$!
sleep 2

# 2. Load Config & Start Engine
echo "2. Configuring..."
./build/bin/yamy-ctl reload --config "$(pwd)/keymaps/master_m00.mayu"
./build/bin/yamy-ctl start
sleep 1

# 3. Run Tests
echo "3. Sending test events via Python/evdev..."
python3 << 'EOF'
import time
import sys
from evdev import InputDevice, ecodes, list_devices

# Target the specific test keyboard that YAMY hooks
target_device = '/dev/input/event3' 

try:
    dev = InputDevice(target_device)
    print(f"Using input device: {dev.name} at {target_device}")
except Exception as e:
    print(f"Error opening device {target_device}: {e}")
    # Try to find any device that isn't the virtual output
    for path in list_devices():
        if path != '/dev/input/event4': # Skip virtual output
             try:
                 d = InputDevice(path)
                 if 'YAMY' in d.name or 'Test' in d.name:
                     dev = d
                     print(f"Fallback to: {dev.name} at {path}")
                     break
             except:
                 pass

def send_key(key_code, value):
    dev.write(ecodes.EV_KEY, key_code, value)
    dev.write(ecodes.EV_SYN, ecodes.SYN_REPORT, 0)

# KEY_B = 48
# KEY_W = 17
# KEY_E = 18
# KEY_R = 19
# KEY_T = 20

# Case 1: TAP B -> Enter (M20 tap action)
print("\n--- Test 1: B Tap -> Enter ---")
send_key(ecodes.KEY_B, 1)
time.sleep(0.05) # < 200ms
send_key(ecodes.KEY_B, 0)
time.sleep(0.5)

# Case 2: HOLD B + W -> 1
print("\n--- Test 2: B Hold + W -> 1 ---")
send_key(ecodes.KEY_B, 1)
time.sleep(0.3) # > 200ms (Hold)
send_key(ecodes.KEY_W, 1)
time.sleep(0.05)
send_key(ecodes.KEY_W, 0)
time.sleep(0.1)
send_key(ecodes.KEY_B, 0)
time.sleep(0.5)

# Case 3: HOLD B + E -> 2
print("\n--- Test 3: B Hold + E -> 2 ---")
send_key(ecodes.KEY_B, 1)
time.sleep(0.3)
send_key(ecodes.KEY_E, 1)
time.sleep(0.05)
send_key(ecodes.KEY_E, 0)
time.sleep(0.1)
send_key(ecodes.KEY_B, 0)
time.sleep(0.5)

# Case 4: Normal Substitution W -> A
print("\n--- Test 4: W Press -> A ---")
send_key(ecodes.KEY_W, 1)
time.sleep(0.05)
send_key(ecodes.KEY_W, 0)
time.sleep(0.5)

print("\nTests complete.")
EOF

sleep 1

# 4. Analyze Results
echo ""
echo "=== LOG ANALYSIS ==="

echo "--- M20 Activation ---"
grep "Hold detected.*ACTIVATE" /tmp/yamy_phase3.log | head -5

echo "--- TAP Output (Enter) ---"
grep "Tap detected.*output" /tmp/yamy_phase3.log | head -5

echo "--- Key Event Processing ---"
# Since detailed debug logs are gone, we rely on ModifierKeyHandler logs
# and journey logs if enabled (they seem to be enabled by default or via env var)
# We can check for "Journey:" if JourneyLogger is active.
grep "Journey:" /tmp/yamy_phase3.log | head -5

# 5. Cleanup
kill -9 $DAEMON_PID 2>/dev/null || true
echo "Done."