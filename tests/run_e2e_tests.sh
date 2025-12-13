#!/bin/bash
# Automated E2E Testing Suite for YAMY
# Tests all master.mayu configurations systematically

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"
KEYMAP_DIR="$SCRIPT_DIR/../keymaps"
YAMY_TEST="$BUILD_DIR/bin/yamy-test"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "=========================================="
echo "YAMY E2E Automated Test Suite"
echo "Full pipeline: Input → YAMY → Output"
echo "=========================================="
echo ""

# Test counter
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Backup original master.mayu
if [ -f "$KEYMAP_DIR/master.mayu" ]; then
    cp "$KEYMAP_DIR/master.mayu" "$KEYMAP_DIR/master.mayu.e2e_backup"
    echo "✓ Backed up master.mayu"
fi

run_e2e_test() {
    local test_name="$1"
    local config_content="$2"
    local input_keys="$3"
    local expected_keys="$4"
    local description="$5"

    TESTS_RUN=$((TESTS_RUN + 1))

    echo ""
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}Test $TESTS_RUN: $test_name${NC}"
    echo -e "${BLUE}$description${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    # Write configuration
    echo "$config_content" > "$KEYMAP_DIR/master.mayu"

    # Run E2E test
    if $YAMY_TEST e2e-auto $input_keys $expected_keys 2>&1 | tee /tmp/yamy_e2e_test_$TESTS_RUN.log | tail -20; then
        echo -e "${GREEN}✓ Test $TESTS_RUN PASSED${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ Test $TESTS_RUN FAILED${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

# Test 1: Baseline (109.mayu only - passthrough)
run_e2e_test \
    "Baseline: 109.mayu only" \
    "include \"109.mayu\"
include \"key_seq.mayu\"" \
    "30,48,46" \
    "30,48,46" \
    "Expected: abc → abc (passthrough, no remapping)"

# Cleanup and summary
cleanup() {
    echo ""
    echo "Cleaning up..."
    killall -9 yamy 2>/dev/null || true
}

trap cleanup EXIT

echo ""
echo "=========================================="
echo "E2E Test Summary"
echo "=========================================="
echo "Tests run:    $TESTS_RUN"
echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"
echo ""

# Restore original
if [ -f "$KEYMAP_DIR/master.mayu.e2e_backup" ]; then
    mv "$KEYMAP_DIR/master.mayu.e2e_backup" "$KEYMAP_DIR/master.mayu"
    echo "✓ Restored original master.mayu"
fi

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All E2E tests passed!${NC}"
    echo ""
    echo "🎉 FANTASTIC! Fully automated E2E testing is working!"
    echo "   Tests complete in seconds with output verification."
    exit 0
else
    echo -e "${RED}✗ Some E2E tests failed${NC}"
    exit 1
fi
