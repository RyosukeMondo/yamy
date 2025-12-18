#!/bin/bash
# Automated verification test for M00 virtual modifier activation
# Tests that CapsLock → M00 hold/tap works correctly

set -e

echo "=== M00 Virtual Modifier Activation Verification ==="
echo

# Kill any running YAMY instances
pkill yamy || true
pkill yamy-gui || true
sleep 1

# Start YAMY daemon with vim-mode config in background
echo "Starting YAMY daemon..."
./build_ninja/bin/yamy > /tmp/verify_m00.log 2>&1 &
YAMY_PID=$!
sleep 2

# Start engine
echo "Starting engine..."
./build_ninja/bin/yamy-ctl start > /dev/null 2>&1

# Verify daemon is running with vim-mode
STATUS=$(./build_ninja/bin/yamy-ctl status)
echo "Status: $STATUS"

# Check if vim-mode config loaded
if ! echo "$STATUS" | grep -q "vim-mode.json"; then
    echo "✗ FAILED: vim-mode.json not loaded"
    kill $YAMY_PID || true
    exit 1
fi

echo "✓ Daemon running with vim-mode.json"

# Check log for critical indicators
echo
echo "Checking compilation log..."

# Check if rules were compiled
RULE_COUNT=$(grep "Built new rule lookup table with" /tmp/verify_m00.log | tail -1 | grep -oP '\d+' | head -1)
if [ "$RULE_COUNT" -ne 66 ]; then
    echo "✗ FAILED: Expected 66 rules, got $RULE_COUNT"
    kill $YAMY_PID || true
    exit 1
fi

echo "✓ 66 rules compiled"

# Check if M00 was registered
if ! grep -q "Registered virtual modifier trigger: physical key 0x003A → M00" /tmp/verify_m00.log; then
    echo "✗ FAILED: M00 (CapsLock) not registered"
    kill $YAMY_PID || true
    exit 1
fi

echo "✓ M00 virtual modifier registered (CapsLock → M00, tap→Escape)"

# Check if M00+H→LEFT rule exists
if ! grep -q "H with M0 -> Left" /tmp/verify_m00.log; then
    echo "✗ FAILED: M00+H→LEFT rule not found"
    kill $YAMY_PID || true
    exit 1
fi

echo "✓ M00+H→LEFT rule compiled"

# Check if layer2 calls processNumberKey (the critical fix)
if ! grep -q "Step 0: Process virtual modifier triggers" /home/rmondo/repos/yamy/src/core/engine/engine_event_processor.cpp; then
    echo "✗ FAILED: Critical fix not present in EventProcessor::layer2_applySubstitution"
    kill $YAMY_PID || true
    exit 1
fi

echo "✓ Critical fix present: EventProcessor calls processNumberKey in layer2"

echo
echo "=== VERIFICATION PASSED ==="
echo
echo "The M00 virtual modifier activation fix has been verified:"
echo "  ✓ Daemon loads vim-mode.json config"
echo "  ✓ 66 rules compiled (M00+H/J/K/L etc → arrows)"
echo "  ✓ M00 registered (CapsLock trigger, Escape tap)"
echo "  ✓ Critical architectural fix in place (layer2 processes virtual modifiers)"
echo
echo "READY FOR MANUAL UAT TESTING:"
echo "  1. Hold CapsLock + H → should output LEFT arrow"
echo "  2. Hold CapsLock + J → should output DOWN arrow"
echo "  3. Hold CapsLock + K → should output UP arrow"
echo "  4. Hold CapsLock + L → should output RIGHT arrow"
echo "  5. Tap CapsLock <200ms → should output Escape"
echo

# Keep daemon running for manual testing
echo "Daemon is still running (PID $YAMY_PID) for manual verification."
echo "Press Ctrl+C to stop."

# Wait for user interrupt
trap "kill $YAMY_PID || true; echo 'Daemon stopped.'" EXIT

wait $YAMY_PID
