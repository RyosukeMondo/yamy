#!/bin/bash
# Automated Testing Script for YAMY
# Minimizes UAT by running systematic tests

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"
YAMY_TEST="$BUILD_DIR/bin/yamy-test"
YAMY_CTL="$BUILD_DIR/bin/yamy-ctl"

echo "=========================================="
echo "YAMY Automated Testing Suite"
echo "=========================================="
echo ""

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counter
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_result="$3"

    TESTS_RUN=$((TESTS_RUN + 1))
    echo -n "Test $TESTS_RUN: $test_name... "

    if eval "$test_command" > /tmp/test_output.log 2>&1; then
        if [ -n "$expected_result" ]; then
            if grep -q "$expected_result" /tmp/test_output.log; then
                echo -e "${GREEN}✓ PASS${NC}"
                TESTS_PASSED=$((TESTS_PASSED + 1))
            else
                echo -e "${RED}✗ FAIL${NC} (expected: $expected_result)"
                TESTS_FAILED=$((TESTS_FAILED + 1))
                cat /tmp/test_output.log
            fi
        else
            echo -e "${GREEN}✓ PASS${NC}"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        fi
    else
        echo -e "${RED}✗ FAIL${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        cat /tmp/test_output.log
    fi
}

echo "1. DRY-RUN TESTS (No actual injection)"
echo "----------------------------------------"

run_test "Dry-run single key A" \
    "$YAMY_TEST dry-run 30" \
    "KEY_A"

run_test "Dry-run sequence ABC" \
    "$YAMY_TEST dry-run 30,48,46" \
    "3 key"

run_test "Dry-run TAB key" \
    "$YAMY_TEST dry-run 15" \
    "KEY_TAB"

echo ""
echo "2. UNIT TESTS (Component validation)"
echo "----------------------------------------"

# Check if YAMY is running
if $YAMY_CTL status > /dev/null 2>&1; then
    YAMY_RUNNING=true
    echo -e "${YELLOW}⚠ YAMY is running - some tests may inject real keys${NC}"
    echo -e "${YELLOW}⚠ Consider stopping YAMY for dry-run only testing${NC}"
else
    YAMY_RUNNING=false
    echo "✓ YAMY is not running - safe for dry-run tests"
fi

echo ""
echo "3. LAYOUT DETECTION TEST"
echo "----------------------------------------"

CURRENT_LAYOUT=$(setxkbmap -query | grep layout | awk '{print $2}')
echo "Current keyboard layout: $CURRENT_LAYOUT"

if [[ "$CURRENT_LAYOUT" == *"jp"* ]]; then
    echo -e "${GREEN}✓ Japanese layout detected${NC}"
elif [[ "$CURRENT_LAYOUT" == *"us"* ]]; then
    echo -e "${GREEN}✓ US layout detected${NC}"
else
    echo -e "${YELLOW}⚠ Unknown layout: $CURRENT_LAYOUT${NC}"
fi

echo ""
echo "4. VIRTUAL KEYBOARD TEST"
echo "----------------------------------------"

# Check if we have permission to create virtual keyboard
if [ -w /dev/uinput ]; then
    echo -e "${GREEN}✓ /dev/uinput is writable${NC}"
else
    echo -e "${YELLOW}⚠ /dev/uinput not writable - run with sudo or add user to input group${NC}"
fi

echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo "Tests run:    $TESTS_RUN"
echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some tests failed${NC}"
    exit 1
fi
