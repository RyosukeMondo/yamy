#!/bin/bash
pkill -9 yamy; pkill -9 python3; sleep 1

# Create test keyboard
python3 << 'EOF' &
from evdev import UInput, ecodes as e
import time
cap = {e.EV_KEY: [e.KEY_A, e.KEY_TAB]}
ui = UInput(cap, name='YAMY-Test-KB')
print("Test keyboard ready", flush=True)
time.sleep(60)
EOF
sleep 2

# Find test device
TEST_DEV=$(cat /proc/bus/input/devices | grep -B 5 "YAMY-Test-KB" | grep "Handlers.*event" | sed 's/.*event\([0-9]*\).*/\1/')
echo "Test keyboard at /dev/input/event${TEST_DEV}"

# Start YAMY
build/bin/yamy 2>&1 > /tmp/yamy.log &
sleep 3
build/bin/yamy-ctl start
sleep 1

# Inject event
echo "Injecting KEY_A..."
python3 << EOF
from evdev import InputDevice, ecodes
dev = InputDevice('/dev/input/event${TEST_DEV}')
dev.write(ecodes.EV_KEY, ecodes.KEY_A, 1)  # Press
dev.write(ecodes.EV_SYN, 0, 0)
import time; time.sleep(0.05)
dev.write(ecodes.EV_KEY, ecodes.KEY_A, 0)  # Release
dev.write(ecodes.EV_SYN, 0, 0)
print("Injected")
EOF

sleep 1

# Check logs
echo "=== Event Logs ==="
grep "\[EVENT\]" /tmp/yamy.log | head -10 || echo "No events logged"

# Cleanup
pkill -9 yamy; pkill -9 python3
