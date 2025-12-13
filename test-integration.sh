#!/bin/bash

# Qt Engine Integration Test Script
# Tests all functionality of the yamy engine integration

set -e

YAMY_BIN="/home/rmondo/repos/yamy/build_release/bin/yamy"
YAMY_CTL="/home/rmondo/repos/yamy/build_release/bin/yamy-ctl"
CONFIG_PATH="/home/rmondo/repos/yamy/keymaps/master.mayu"
TEST_LOG="/tmp/yamy-test-$(date +%s).log"

echo "======================================"
echo "YAMY Qt Engine Integration Test"
echo "======================================"
echo ""
echo "Test log: $TEST_LOG"
echo ""

# Function to print test results
test_result() {
    if [ $1 -eq 0 ]; then
        echo "✅ PASS: $2"
    else
        echo "❌ FAIL: $2"
        echo "  Error: $3"
    fi
}

# Function to cleanup
cleanup() {
    echo ""
    echo "Cleaning up..."
    pkill -f "$YAMY_BIN" 2>/dev/null || true
    sleep 1
    rm -f /tmp/yamy-control.sock
    echo "Cleanup complete"
}

trap cleanup EXIT

# Test 1: Binary exists and responds to --version
echo "Test 1: Binary verification"
if $YAMY_BIN --version 2>&1 | grep -q "YAMY"; then
    test_result 0 "Binary exists and shows version"
else
    test_result 1 "Binary version check" "Version command failed"
    exit 1
fi

# Test 2: Start yamy daemon
echo ""
echo "Test 2: Starting yamy daemon"
echo "Starting yamy in background..."
$YAMY_BIN --no-restore > $TEST_LOG 2>&1 &
YAMY_PID=$!
sleep 3  # Give it time to initialize

if ps -p $YAMY_PID > /dev/null; then
    test_result 0 "Yamy process started (PID: $YAMY_PID)"
else
    test_result 1 "Yamy process start" "Process died immediately"
    cat $TEST_LOG
    exit 1
fi

# Test 3: IPC socket creation
echo ""
echo "Test 3: IPC socket verification"
if [ -S /tmp/yamy-control.sock ]; then
    test_result 0 "IPC control socket exists"
else
    test_result 1 "IPC socket creation" "Socket not found"
    exit 1
fi

# Test 4: IPC status command
echo ""
echo "Test 4: IPC status command"
STATUS_OUTPUT=$($YAMY_CTL status 2>&1)
echo "Status output: $STATUS_OUTPUT"
if echo "$STATUS_OUTPUT" | grep -q "Engine:"; then
    test_result 0 "Status command returns engine state"
else
    test_result 1 "Status command" "No engine state in output"
fi

# Test 5: Load configuration
echo ""
echo "Test 5: Configuration loading"
if [ -f "$CONFIG_PATH" ]; then
    RELOAD_OUTPUT=$($YAMY_CTL reload "$CONFIG_PATH" 2>&1)
    echo "Reload output: $RELOAD_OUTPUT"
    if echo "$RELOAD_OUTPUT" | grep -qi "success\|loaded"; then
        test_result 0 "Configuration loaded successfully"
        sleep 1  # Give engine time to process
    else
        test_result 1 "Configuration loading" "$RELOAD_OUTPUT"
    fi
else
    test_result 1 "Configuration file check" "Config file not found: $CONFIG_PATH"
fi

# Test 6: Verify config is active
echo ""
echo "Test 6: Configuration verification"
CONFIG_OUTPUT=$($YAMY_CTL config 2>&1)
echo "Config output: $CONFIG_OUTPUT"
if echo "$CONFIG_OUTPUT" | grep -q "master.mayu"; then
    test_result 0 "Active configuration shows master.mayu"
else
    test_result 1 "Configuration verification" "Config path not found in output"
fi

# Test 7: Get keymaps
echo ""
echo "Test 7: Keymaps query"
KEYMAPS_OUTPUT=$($YAMY_CTL keymaps 2>&1)
echo "Keymaps output (first 200 chars): ${KEYMAPS_OUTPUT:0:200}..."
if echo "$KEYMAPS_OUTPUT" | grep -q "keymaps"; then
    test_result 0 "Keymaps command returns data"
else
    test_result 1 "Keymaps query" "No keymaps data returned"
fi

# Test 8: Performance metrics
echo ""
echo "Test 8: Performance metrics"
METRICS_OUTPUT=$($YAMY_CTL metrics 2>&1)
echo "Metrics output: $METRICS_OUTPUT"
if echo "$METRICS_OUTPUT" | grep -q "latency\|keys_per_second"; then
    test_result 0 "Metrics command returns performance data"
else
    test_result 1 "Metrics query" "No metrics data returned"
fi

# Test 9: Start engine
echo ""
echo "Test 9: Engine start command"
START_OUTPUT=$($YAMY_CTL start 2>&1)
echo "Start output: $START_OUTPUT"
sleep 1
STATUS_AFTER_START=$($YAMY_CTL status 2>&1)
echo "Status after start: $STATUS_AFTER_START"
if echo "$STATUS_AFTER_START" | grep -qi "running"; then
    test_result 0 "Engine started successfully"
else
    test_result 1 "Engine start" "Engine not running after start command"
fi

# Test 10: Stop engine
echo ""
echo "Test 10: Engine stop command"
STOP_OUTPUT=$($YAMY_CTL stop 2>&1)
echo "Stop output: $STOP_OUTPUT"
sleep 1
STATUS_AFTER_STOP=$($YAMY_CTL status 2>&1)
echo "Status after stop: $STATUS_AFTER_STOP"
if echo "$STATUS_AFTER_STOP" | grep -qi "stopped"; then
    test_result 0 "Engine stopped successfully"
else
    test_result 1 "Engine stop" "Engine still running after stop command"
fi

# Test 11: Check debug log for errors
echo ""
echo "Test 11: Error log verification"
if [ -f /tmp/yamy-debug.log ]; then
    ERROR_COUNT=$(grep -ci "error\|exception\|failed" /tmp/yamy-debug.log 2>/dev/null || echo "0")
    echo "Debug log errors found: $ERROR_COUNT"
    if [ "$ERROR_COUNT" -lt 5 ]; then
        test_result 0 "Debug log shows minimal errors ($ERROR_COUNT)"
    else
        test_result 1 "Error log check" "Too many errors in debug log ($ERROR_COUNT)"
        echo "Recent errors:"
        grep -i "error\|exception\|failed" /tmp/yamy-debug.log | tail -5
    fi
else
    test_result 1 "Debug log check" "Debug log not found"
fi

# Test 12: Session file creation
echo ""
echo "Test 12: Session persistence"
SESSION_FILE="$HOME/.config/yamy/session.json"
if [ -f "$SESSION_FILE" ]; then
    test_result 0 "Session file created at $SESSION_FILE"
    echo "Session content:"
    cat "$SESSION_FILE" | head -10
else
    test_result 1 "Session file check" "Session file not found"
fi

echo ""
echo "======================================"
echo "Test Summary"
echo "======================================"
echo "✅ Tests completed"
echo "Yamy PID: $YAMY_PID"
echo "Full log: $TEST_LOG"
echo ""
echo "To manually test GUI features:"
echo "  1. Check system tray for yamy icon"
echo "  2. Right-click icon → Settings"
echo "  3. Load config and verify notifications"
echo ""
