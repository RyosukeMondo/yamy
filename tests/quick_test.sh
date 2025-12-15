#!/bin/bash
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# quick_test.sh - Rapid iteration test runner
# Ultra-fast test execution with excellent logging

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="${BUILD_DIR:-build}"
TEST_RUNNER="$BUILD_DIR/bin/yamy-test-runner"
YAMY_DAEMON="$BUILD_DIR/bin/yamy"
YAMY_CTL="$BUILD_DIR/bin/yamy-ctl"

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  YAMY Quick Test Runner - Rapid Iteration Mode${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# Check if tools exist
if [ ! -f "$TEST_RUNNER" ]; then
    echo -e "${RED}✗ Test runner not found: $TEST_RUNNER${NC}"
    echo -e "${YELLOW}  Run: cmake --build $BUILD_DIR${NC}"
    exit 1
fi

# Function: Check if YAMY is running
check_yamy() {
    if pgrep -x yamy > /dev/null; then
        return 0
    else
        return 1
    fi
}

# Function: Start YAMY daemon
start_yamy() {
    echo -e "${YELLOW}Starting YAMY daemon...${NC}"

    # Kill any existing instance
    pkill -9 yamy 2>/dev/null || true
    sleep 0.5

    # Start daemon in background
    YAMY_DEBUG_KEYCODE=1 $YAMY_DAEMON > /tmp/yamy_test.log 2>&1 &
    YAMY_PID=$!
    sleep 2

    # Start engine
    if [ -f "$YAMY_CTL" ]; then
        $YAMY_CTL start 2>&1 || echo -e "${YELLOW}  Warning: yamy-ctl start failed${NC}"
        sleep 1
    fi

    echo -e "${GREEN}✓ YAMY daemon started (PID: $YAMY_PID)${NC}"
    echo ""
}

# Function: Stop YAMY daemon
stop_yamy() {
    echo ""
    echo -e "${YELLOW}Stopping YAMY daemon...${NC}"
    pkill -9 yamy 2>/dev/null || true
    echo -e "${GREEN}✓ YAMY daemon stopped${NC}"
}

# Function: Run test scenario
run_test() {
    local scenario=$1
    local test_name=$(basename "$scenario" .json)

    echo -e "${BLUE}───────────────────────────────────────────────────────────${NC}"
    echo -e "${BLUE}Test: $test_name${NC}"
    echo -e "${BLUE}───────────────────────────────────────────────────────────${NC}"

    # Run test
    if $TEST_RUNNER --scenario "$scenario" --report "/tmp/yamy_test_${test_name}.json"; then
        echo -e "${GREEN}✓ PASSED${NC}"

        # Show summary
        if [ -f "/tmp/yamy_test_${test_name}.json" ]; then
            echo -e "${BLUE}Summary:${NC}"
            cat "/tmp/yamy_test_${test_name}.json" | jq -r '.test_case_results[] | "  \(.name): \(.status) (\(.duration_ms)ms)"' 2>/dev/null || true
        fi

        return 0
    else
        echo -e "${RED}✗ FAILED${NC}"

        # Show detailed failure info
        if [ -f "/tmp/yamy_test_${test_name}.json" ]; then
            echo -e "${RED}Failures:${NC}"
            cat "/tmp/yamy_test_${test_name}.json" | jq -r '.test_case_results[] | select(.status != "PASSED") | "  \(.name): \(.error_message)"' 2>/dev/null || true
        fi

        return 1
    fi
}

# Trap to ensure cleanup
trap stop_yamy EXIT

# Start YAMY
if ! check_yamy; then
    start_yamy
else
    echo -e "${GREEN}✓ YAMY already running${NC}"
    echo ""
fi

# Run tests
FAILED=0
PASSED=0

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Running Test Scenarios${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# Test scenarios in order of priority
TESTS=(
    "tests/scenarios/phase1_atomic/00_quick_smoke.json"
    "tests/scenarios/phase2_modal/00_m0_critical.json"
)

for test in "${TESTS[@]}"; do
    if [ -f "$test" ]; then
        if run_test "$test"; then
            ((PASSED++))
        else
            ((FAILED++))
        fi
        echo ""
    else
        echo -e "${YELLOW}⚠ Test not found: $test${NC}"
        echo ""
    fi
done

# Summary
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Test Summary${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "Total tests: $((PASSED + FAILED))"
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"
echo ""

# Show YAMY log excerpt
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  YAMY Log (last 20 lines)${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
tail -20 /tmp/yamy_test.log 2>/dev/null || echo "No log available"
echo ""

# Exit with failure if any tests failed
if [ $FAILED -gt 0 ]; then
    echo -e "${RED}✗ Some tests failed${NC}"
    exit 1
else
    echo -e "${GREEN}✓ All tests passed!${NC}"
    exit 0
fi
