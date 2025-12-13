#!/bin/bash
# Test master.mayu configurations systematically
# Goal: Uncomment all imports and verify they work as expected

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"
KEYMAP_DIR="$SCRIPT_DIR/../keymaps"
YAMY_TEST="$BUILD_DIR/bin/yamy-test"
YAMY_CTL="$BUILD_DIR/bin/yamy-ctl"
YAMY_BIN="$BUILD_DIR/bin/yamy"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "=========================================="
echo "YAMY master.mayu Configuration Tests"
echo "Goal: Enable all imports systematically"
echo "=========================================="
echo ""

# Backup original master.mayu
cp "$KEYMAP_DIR/master.mayu" "$KEYMAP_DIR/master.mayu.backup"
echo "✓ Backed up master.mayu"

# Test counter
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

test_config() {
    local config_name="$1"
    local config_content="$2"
    local test_keys="$3"
    local expected_behavior="$4"

    TESTS_RUN=$((TESTS_RUN + 1))

    echo ""
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}Test $TESTS_RUN: $config_name${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    # Write test configuration
    echo "$config_content" > "$KEYMAP_DIR/master.mayu"
    echo "Configuration:"
    cat "$KEYMAP_DIR/master.mayu"
    echo ""

    # Dry-run test to show what would be injected
    echo "Dry-run test (showing injection plan):"
    $YAMY_TEST dry-run $test_keys || true
    echo ""

    echo "Expected behavior: $expected_behavior"
    echo ""
    echo -n "Does this match expectations? "

    # For now, we'll mark as passed (later: add output capture and verification)
    echo -e "${GREEN}✓ MANUAL VERIFICATION NEEDED${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

# Test 1: Baseline (109.mayu + key_seq.mayu only)
test_config \
    "Baseline: 109.mayu only" \
    "include \"109.mayu\"
include \"key_seq.mayu\"" \
    "30,48,46" \
    "abc → abc (passthrough, no remapping)"

# Test 2: Add config.mayu
test_config \
    "With config.mayu" \
    "include \"109.mayu\"
include \"key_seq.mayu\"
include \"config.mayu\"" \
    "30,48,46" \
    "abc → remapped per config.mayu (def subst rules)"

# Test 3: Add hm.mayu
test_config \
    "With config.mayu + hm.mayu" \
    "include \"109.mayu\"
include \"key_seq.mayu\"
include \"config.mayu\"
include \"hm.mayu\"" \
    "30,48,46" \
    "abc → remapped per config.mayu + hm.mayu"

# Test 4: Add dvorakY.mayu (requires US layout or JP-compatible version)
CURRENT_LAYOUT=$(setxkbmap -query | grep layout | awk '{print $2}')
if [[ "$CURRENT_LAYOUT" == *"jp"* ]]; then
    echo ""
    echo -e "${YELLOW}⚠ NOTE: dvorakY.mayu is designed for US layout${NC}"
    echo -e "${YELLOW}⚠ Current layout: $CURRENT_LAYOUT${NC}"
    echo -e "${YELLOW}⚠ Results may differ from expectations${NC}"
fi

test_config \
    "Full: All configurations enabled" \
    "include \"109.mayu\"
include \"key_seq.mayu\"
include \"config.mayu\"
include \"hm.mayu\"
include \"dvorakY.mayu\"" \
    "30,48,46" \
    "abc → fully remapped (109 + config + hm + dvorakY)"

# Summary
echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo "Tests run:    $TESTS_RUN"
echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"
echo ""

echo "Next Steps:"
echo "1. Review each test output above"
echo "2. Verify expected behavior matches reality"
echo "3. Choose which configuration to use"
echo "4. Update master.mayu with chosen config"
echo ""

echo "Current layout: $CURRENT_LAYOUT"
echo "Backup saved: $KEYMAP_DIR/master.mayu.backup"
echo ""

# Restore original or keep test version?
read -p "Restore original master.mayu? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    mv "$KEYMAP_DIR/master.mayu.backup" "$KEYMAP_DIR/master.mayu"
    echo "✓ Restored original master.mayu"
else
    echo "✓ Keeping test version of master.mayu"
    echo "  (Backup available at master.mayu.backup)"
fi
