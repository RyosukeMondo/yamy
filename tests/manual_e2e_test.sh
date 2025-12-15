#!/bin/bash
set -e

echo "=== YAMY E2E Test ==="

# Cleanup
pkill -9 yamy 2>/dev/null || true
pkill -9 -f test_keyboard 2>/dev/null || true
sleep 1

# Step 1: Create test keyboard device
echo "Step 1: Creating test keyboard device..."
python3 tests/test_keyboard_daemon.py "YAMY-Test-Keyboard" 2>&1 | grep -m1 READY &
sleep 3

# Verify device exists
if cat /proc/bus/input/devices | grep -q "YAMY-Test-Keyboard"; then
    echo "✓ Test keyboard created"
    TEST_DEV=$(cat /proc/bus/input/devices | grep -B 5 "YAMY-Test-Keyboard" | grep "Handlers.*event" | sed 's/.*event\([0-9]*\).*/\1/')
    echo "  Device: /dev/input/event${TEST_DEV}"
else
    echo "✗ Failed to create test keyboard"
    exit 1
fi

# Step 2: Start YAMY daemon
echo "Step 2: Starting YAMY daemon..."
build/bin/yamy 2>&1 > /tmp/yamy_e2e.log &
sleep 3

# Start engine
build/bin/yamy-ctl start 2>&1
sleep 1

# Check if YAMY grabbed the test device
if grep -q "Processing keyboard.*YAMY-Test-Keyboard" /tmp/yamy_e2e.log; then
    echo "✓ YAMY found test keyboard"
else
    echo "✗ YAMY didn't find test keyboard"
    cat /tmp/yamy_e2e.log | grep -i "Processing keyboard"
    exit 1
fi

if grep -q "Successfully hooked.*event${TEST_DEV}" /tmp/yamy_e2e.log; then
    echo "✓ YAMY grabbed test keyboard"
else
    echo "⚠ YAMY may not have grabbed test keyboard"
fi

# Step 3: Inject event and capture output
echo "Step 3: Injecting KEY_A and capturing output..."
build/bin/yamy-capture --timeout 2000 > /tmp/capture_output.txt &
CAPTURE_PID=$!
sleep 0.5

# Inject KEY_A directly to the test device
python3 << 'EOF'
from evdev import InputDevice, ecodes
import time

dev = InputDevice('/dev/input/event'+'${TEST_DEV}')
# KEY_A press
dev.write(ecodes.EV_KEY, ecodes.KEY_A, 1)
dev.write(ecodes.EV_SYN, 0, 0)
time.sleep(0.05)
# KEY_A release
dev.write(ecodes.EV_KEY, ecodes.KEY_A, 0)
dev.write(ecodes.EV_SYN, 0, 0)
print("Injected KEY_A")
EOF

sleep 1.5
wait $CAPTURE_PID 2>/dev/null || true

# Check results
CAPTURED=$(cat /tmp/capture_output.txt | grep -c "captured_events" || echo 0)
if [ "$CAPTURED" -gt 0 ]; then
    echo "✓ Events captured!"
    cat /tmp/capture_output.txt
else
    echo "✗ No events captured"
fi

# Cleanup
pkill -9 yamy 2>/dev/null || true
pkill -9 -f test_keyboard 2>/dev/null || true

echo "=== Test Complete ==="
