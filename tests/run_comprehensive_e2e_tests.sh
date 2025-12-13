#!/bin/bash
# Comprehensive E2E Test Suite for YAMY
# Automatically runs all test cases from e2e_test_cases.txt

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"
KEYMAP_DIR="$SCRIPT_DIR/../keymaps"
YAMY_TEST="$BUILD_DIR/bin/yamy-test"
TEST_CASES="$SCRIPT_DIR/e2e_test_cases.txt"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo "=========================================="
echo "YAMY Comprehensive E2E Test Suite"
echo "=========================================="
echo ""

# Test counter
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0

# Backup original master.mayu
if [ -f "$KEYMAP_DIR/master.mayu" ]; then
    cp "$KEYMAP_DIR/master.mayu" "$KEYMAP_DIR/master.mayu.e2e_backup"
    echo "✓ Backed up master.mayu"
fi

cleanup() {
    echo ""
    echo "Cleaning up..."
    killall -9 yamy 2>/dev/null || true

    # Restore original
    if [ -f "$KEYMAP_DIR/master.mayu.e2e_backup" ]; then
        mv "$KEYMAP_DIR/master.mayu.e2e_backup" "$KEYMAP_DIR/master.mayu"
        echo "✓ Restored original master.mayu"
    fi
}

trap cleanup EXIT

# Generate master.mayu from config name
generate_config() {
    local config_name="$1"
    local config_content=""

    case "$config_name" in
        "109.mayu")
            config_content="include \"109.mayu\"
include \"key_seq.mayu\""
            ;;
        "config.mayu")
            config_content="include \"109.mayu\"
include \"key_seq.mayu\"
include \"config.mayu\""
            ;;
        "config+hm")
            config_content="include \"109.mayu\"
include \"key_seq.mayu\"
include \"config.mayu\"
include \"hm.mayu\""
            ;;
        "dvorakY")
            config_content="include \"109.mayu\"
include \"key_seq.mayu\"
include \"dvorakY.mayu\""
            ;;
        "config+hm+dvorakY")
            config_content="include \"109.mayu\"
include \"key_seq.mayu\"
include \"config.mayu\"
include \"hm.mayu\"
include \"dvorakY.mayu\""
            ;;
        *)
            echo "Unknown config: $config_name"
            return 1
            ;;
    esac

    echo "$config_content"
}

run_e2e_test() {
    local test_name="$1"
    local config="$2"
    local input_keys="$3"
    local expected_keys="$4"
    local description="$5"

    TESTS_RUN=$((TESTS_RUN + 1))

    # Check if test requires features not yet implemented
    if [[ "$input_keys" == *"HOLD:"* ]] || [[ "$input_keys" == *"SHIFT:"* ]]; then
        echo -e "${CYAN}⊘ Test $TESTS_RUN: $test_name - SKIPPED (requires key hold)${NC}"
        TESTS_SKIPPED=$((TESTS_SKIPPED + 1))
        return
    fi

    if [[ "$expected_keys" == *"SHIFT:"* ]] || [[ "$expected_keys" == *"GRAVE"* ]] || [[ "$expected_keys" == *"COMMERCIAL_AT"* ]]; then
        echo -e "${CYAN}⊘ Test $TESTS_RUN: $test_name - SKIPPED (symbol output not yet supported)${NC}"
        TESTS_SKIPPED=$((TESTS_SKIPPED + 1))
        return
    fi

    echo ""
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}Test $TESTS_RUN: $test_name${NC}"
    echo -e "${BLUE}$description${NC}"
    echo -e "${BLUE}Config: $config${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    # Generate and write configuration
    local config_content=$(generate_config "$config")
    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ Test $TESTS_RUN FAILED - Invalid config${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return
    fi

    echo "$config_content" > "$KEYMAP_DIR/master.mayu"

    # Run E2E test (redirect stderr to suppress verbose output)
    if $YAMY_TEST e2e-auto $input_keys $expected_keys 2>&1 | grep -q "✓ PASSED"; then
        echo -e "${GREEN}✓ Test $TESTS_RUN PASSED${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ Test $TESTS_RUN FAILED${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        # Show last few lines of output for debugging
        $YAMY_TEST e2e-auto $input_keys $expected_keys 2>&1 | tail -10
    fi
}

# Parse and run tests from e2e_test_cases.txt
echo "Loading test cases from: $TEST_CASES"
echo ""

while IFS='|' read -r test_name config input_keys expected_keys description; do
    # Skip comments and empty lines
    [[ "$test_name" =~ ^#.*$ ]] && continue
    [[ -z "$test_name" ]] && continue

    # Trim whitespace
    test_name=$(echo "$test_name" | xargs)
    config=$(echo "$config" | xargs)
    input_keys=$(echo "$input_keys" | xargs)
    expected_keys=$(echo "$expected_keys" | xargs)
    description=$(echo "$description" | xargs)

    # Skip if any field is empty
    [[ -z "$test_name" ]] && continue

    run_e2e_test "$test_name" "$config" "$input_keys" "$expected_keys" "$description"
done < "$TEST_CASES"

# Summary
echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo "Tests run:     $TESTS_RUN"
echo -e "Tests passed:  ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests failed:  ${RED}$TESTS_FAILED${NC}"
echo -e "Tests skipped: ${CYAN}$TESTS_SKIPPED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All executed tests passed!${NC}"
    if [ $TESTS_SKIPPED -gt 0 ]; then
        echo -e "${CYAN}ⓘ $TESTS_SKIPPED test(s) skipped (require unimplemented features)${NC}"
    fi
    exit 0
else
    echo -e "${RED}✗ Some tests failed${NC}"
    exit 1
fi
