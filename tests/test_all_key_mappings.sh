#!/bin/bash
# Comprehensive Key Mapping Test - Tests ALL substitutions from config.mayu
# Shows Layer 1 → Layer 2 → Layer 3 transformations for every key

set -e

BUILD_DIR="../build"
KEYMAP_DIR="../keymaps"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=============================================="
echo "YAMY Comprehensive Key Mapping Test"
echo "=============================================="
echo ""
echo "Testing ALL key substitutions from config.mayu"
echo "Format: Input Key → Expected Output"
echo ""

# Define all key mappings from config.mayu
# Format: "input_name:input_evdev:expected_name:expected_evdev"
declare -a TESTS=(
    # Letters that you confirmed working
    "W:17:A:30"
    "E:18:O:24"
    "R:19:E:18"
    "T:20:U:22"
    "Y:21:I:23"

    # Other letter substitutions from config.mayu
    "A:30:Tab:15"
    "B:48:Enter:28"
    "V:47:BS:14"
    "Q:16:Minus:12"
    "U:22:D:32"
    "I:23:H:35"
    "O:24:T:20"
    "P:25:N:49"
    "S:31:Semicolon:39"
    "D:32:Q:16"
    "F:33:J:36"
    "G:34:K:37"
    "H:35:X:45"
    "J:36:B:48"
    "K:37:M:50"
    "L:38:W:17"
    "Z:44:Z:44"
    "X:45:_3:4"
    "C:46:Del:111"
    "N:49:LShift:42"
    "M:50:BS:14"

    # Numbers (passthrough in config.mayu, but may have substitutions)
    "1:2:LShift:42"
    "2:3:Colon:39"
    "3:4:Comma:51"
    "0:11:R:19"

    # Special keys
    "Tab:15:Space:57"
    "Enter:28:Yen:124"
    "Esc:1:_5:6"

    # Function keys
    "F1:59:LWin:125"
    "F2:60:Esc:1"
    "F3:61:LCtrl:29"
    "F5:63:BS:14"
    "F6:64:Del:111"
    "F8:66:Tab:15"
    "F9:67:Tab:15"
    "F10:68:Tab:15"
    "F11:87:Tab:15"
    "F12:88:Tab:15"
)

TOTAL=${#TESTS[@]}
PASSED=0
FAILED=0

echo "Testing $TOTAL key mappings..."
echo ""

# Create test results file
RESULTS_FILE="/tmp/yamy_mapping_results.txt"
> "$RESULTS_FILE"

for test in "${TESTS[@]}"; do
    IFS=':' read -r input_name input_evdev expected_name expected_evdev <<< "$test"

    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" | tee -a "$RESULTS_FILE"
    echo "Test: $input_name ($input_evdev) → $expected_name ($expected_evdev)" | tee -a "$RESULTS_FILE"

    # Clear debug log
    > /tmp/yamy_debug.log

    # Inject key and capture output
    result=$($BUILD_DIR/bin/yamy-test inject $input_evdev 2>&1 || true)

    # Wait a moment for processing
    sleep 0.1

    # Check debug log for layer transformations
    layer1=$(grep "\[LAYER1:IN\]" /tmp/yamy_debug.log 2>/dev/null | tail -1 || echo "")
    layer2=$(grep "\[LAYER2:OUT\]" /tmp/yamy_debug.log 2>/dev/null | tail -1 || echo "")
    layer3=$(grep "\[LAYER3:OUT\]" /tmp/yamy_debug.log 2>/dev/null | tail -1 || echo "")

    echo "  Layer 1: $layer1" | tee -a "$RESULTS_FILE"
    echo "  Layer 2: $layer2" | tee -a "$RESULTS_FILE"
    echo "  Layer 3: $layer3" | tee -a "$RESULTS_FILE"

    # Try to capture actual output (this is approximate)
    # For now, we check if the expected evdev appears in Layer 3
    if echo "$layer3" | grep -q "evdev $expected_evdev"; then
        echo -e "  ${GREEN}✓ PASSED${NC}" | tee -a "$RESULTS_FILE"
        ((PASSED++))
    else
        echo -e "  ${RED}✗ FAILED${NC}" | tee -a "$RESULTS_FILE"
        ((FAILED++))
    fi
    echo "" | tee -a "$RESULTS_FILE"
done

echo "=============================================="
echo "Test Summary"
echo "=============================================="
echo -e "Total:  $TOTAL"
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"
echo ""
echo "Full results saved to: $RESULTS_FILE"

if [ $FAILED -gt 0 ]; then
    echo ""
    echo "Failed tests:"
    grep -B1 "✗ FAILED" "$RESULTS_FILE" | grep "^Test:" || true
fi
