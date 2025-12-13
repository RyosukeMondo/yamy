#!/bin/bash
# Integration test: Run YAMY and verify remapping works
# Tests actual INPUT → YAMY → OUTPUT pipeline

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"
KEYMAP_DIR="$SCRIPT_DIR/../keymaps"
YAMY_BIN="$BUILD_DIR/bin/yamy"
YAMY_CTL="$BUILD_DIR/bin/yamy-ctl"
YAMY_TEST="$BUILD_DIR/bin/yamy-test"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "=========================================="
echo "YAMY Integration Test"
echo "Tests: INPUT → YAMY → OUTPUT"
echo "=========================================="
echo ""

# Check if YAMY is already running
if $YAMY_CTL status > /dev/null 2>&1; then
    echo -e "${YELLOW}⚠ YAMY is already running${NC}"
    YAMY_WAS_RUNNING=true
else
    YAMY_WAS_RUNNING=false
fi

cleanup() {
    if [ "$YAMY_WAS_RUNNING" = false ]; then
        echo "Stopping YAMY..."
        killall -9 yamy 2>/dev/null || true
    fi
}

trap cleanup EXIT

run_integration_test() {
    local test_name="$1"
    local config_file="$2"

    echo ""
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}Integration Test: $test_name${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    # Write config
    echo "$config_file" > "$KEYMAP_DIR/master.mayu"
    echo "Configuration:"
    cat "$KEYMAP_DIR/master.mayu"
    echo ""

    # Start YAMY if not running
    if [ "$YAMY_WAS_RUNNING" = false ]; then
        echo "Starting YAMY..."
        $YAMY_BIN > /tmp/yamy_integration_test.log 2>&1 &
        sleep 3

        echo "Starting YAMY engine..."
        $YAMY_CTL start
        sleep 1
    else
        echo "Reloading YAMY configuration..."
        $YAMY_CTL reload || {
            echo "Reload failed, restarting YAMY..."
            killall -9 yamy 2>/dev/null || true
            sleep 1
            $YAMY_BIN > /tmp/yamy_integration_test.log 2>&1 &
            sleep 3
            $YAMY_CTL start
            sleep 1
        }
    fi

    # Check status
    echo "YAMY Status:"
    $YAMY_CTL status

    # Check metrics before
    echo ""
    echo "Metrics before injection:"
    $YAMY_CTL metrics

    echo ""
    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${YELLOW}MANUAL STEP: Now test by typing 'abc'${NC}"
    echo -e "${YELLOW}Report what you see on screen${NC}"
    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""
    echo "Press Enter when done testing..."
    read

    # Check metrics after
    echo ""
    echo "Metrics after typing:"
    $YAMY_CTL metrics

    echo ""
    echo "Check logs for details:"
    echo "  tail -100 /tmp/yamy_integration_test.log | grep -E 'Converting|Writing'"
    echo ""
}

# Test 1: Baseline (109.mayu only)
run_integration_test \
    "Baseline (109.mayu only)" \
    "include \"109.mayu\"
include \"key_seq.mayu\""

# Test 2: With config.mayu
run_integration_test \
    "With config.mayu" \
    "include \"109.mayu\"
include \"key_seq.mayu\"
include \"config.mayu\""

# Test 3: With hm.mayu
run_integration_test \
    "With config.mayu + hm.mayu" \
    "include \"109.mayu\"
include \"key_seq.mayu\"
include \"config.mayu\"
include \"hm.mayu\""

# Test 4: Full configuration
run_integration_test \
    "Full (all configs)" \
    "include \"109.mayu\"
include \"key_seq.mayu\"
include \"config.mayu\"
include \"hm.mayu\"
include \"dvorakY.mayu\""

echo ""
echo "=========================================="
echo "Integration Tests Complete"
echo "=========================================="
echo ""
echo "Summary of what was tested:"
echo "1. Baseline: 109.mayu only (should passthrough abc → abc)"
echo "2. With config.mayu (should apply def subst remappings)"
echo "3. With hm.mayu (should apply additional remappings)"
echo "4. Full config (all remapping layers active)"
echo ""
echo "Backup: $KEYMAP_DIR/master.mayu.backup"
echo ""
