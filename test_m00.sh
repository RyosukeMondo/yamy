#!/bin/bash
# Fast iteration test script for M00 debugging

set -e

echo "=== M00 Test Script ==="
echo "1. Building..."
cmake --build build --target yamy -j8 2>&1 | tail -3

echo "2. Stopping old daemon..."
pkill -9 yamy 2>/dev/null || true
sleep 0.5

echo "3. Starting daemon with full logging..."
./build/bin/yamy > /tmp/yamy_test.log 2>&1 &
DAEMON_PID=$!
sleep 2

echo "4. Starting engine..."
./build/bin/yamy-ctl start
sleep 1

echo "5. Sending test events..."
python3 << 'EOF'
from evdev import InputDevice, ecodes
import time
dev = InputDevice('/dev/input/event3')

# Test 1: Single B tap
print("Test 1: B tap")
dev.write(ecodes.EV_KEY, ecodes.KEY_B, 1)
dev.write(ecodes.EV_SYN, ecodes.SYN_REPORT, 0)
time.sleep(0.05)
dev.write(ecodes.EV_KEY, ecodes.KEY_B, 0)
dev.write(ecodes.EV_SYN, ecodes.SYN_REPORT, 0)
time.sleep(0.3)

# Test 2: B hold + A press
print("Test 2: B hold (700ms) + A")
dev.write(ecodes.EV_KEY, ecodes.KEY_B, 1)
dev.write(ecodes.EV_SYN, ecodes.SYN_REPORT, 0)
time.sleep(0.7)
dev.write(ecodes.EV_KEY, ecodes.KEY_A, 1)
dev.write(ecodes.EV_SYN, ecodes.SYN_REPORT, 0)
time.sleep(0.05)
dev.write(ecodes.EV_KEY, ecodes.KEY_A, 0)
dev.write(ecodes.EV_SYN, ecodes.SYN_REPORT, 0)
time.sleep(0.1)
dev.write(ecodes.EV_KEY, ecodes.KEY_B, 0)
dev.write(ecodes.EV_SYN, ecodes.SYN_REPORT, 0)
print("Done")
EOF

sleep 1

echo "6. Analyzing logs..."
echo ""
echo "=== CONFIG LOADING ==="
grep -E "BUILD_SUBST|Number of sub|Substitution: 0x" /tmp/yamy_test.log | head -20

echo ""
echo "=== EVENT FLOW ==="
grep -E "EVENT.*scancode.*0x30|EVENT.*scancode.*0x1e|LAYER2:SUBST|OUTPUT.*0x" /tmp/yamy_test.log | tail -20

echo ""
echo "=== MODIFIER STATE ==="
grep -E "Hold detected|ACTIVATE|virtual.*mod|M00" /tmp/yamy_test.log | tail -10

echo ""
echo "7. Stopping daemon..."
kill -9 $DAEMON_PID 2>/dev/null || true

echo ""
echo "=== SUMMARY ==="
echo "Substitutions loaded: $(grep -c 'Built substitution table' /tmp/yamy_test.log)"
echo "B key events: $(grep -c 'scancode.*0x30' /tmp/yamy_test.log)"
echo "A key events: $(grep -c 'scancode.*0x1e' /tmp/yamy_test.log)"
echo "Full log: /tmp/yamy_test.log"
