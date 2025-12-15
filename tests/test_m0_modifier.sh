#!/bin/bash
# Automated test for M0 modal modifier with B+WOEU → 1234

set -e

BUILD_DIR="build/linux-debug"
CONFIG="keymaps/master_clean.mayu"

echo "=========================================="
echo "M0 Modal Modifier Test (B+WOEU → 1234)"
echo "=========================================="
echo ""

# Reload config
echo "Reloading config: $CONFIG"
$BUILD_DIR/bin/yamy-ctl reload --config "$CONFIG"
sleep 0.5

echo ""
echo "Testing M0 modal modifier bindings:"
echo "  Hold B (mod0) + W → should output 1"
echo "  Hold B (mod0) + O → should output 2"
echo "  Hold B (mod0) + E → should output 3"
echo "  Hold B (mod0) + U → should output 4"
echo ""

# Check if yamy-test exists
if [ ! -f "$BUILD_DIR/bin/yamy-test" ]; then
    echo "ERROR: yamy-test binary not found!"
    echo "Building test binary..."
    cmake --build $BUILD_DIR --target yamy-test
fi

# Test function
test_modal_combo() {
    local modifier_key=$1
    local modifier_name=$2
    local test_key=$3
    local test_name=$4
    local expected=$5

    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Test: $modifier_name + $test_name → $expected"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    # Simulate: Press modifier, press key, release key, release modifier
    echo "Injecting: Press $modifier_name($modifier_key) → Press $test_name($test_key) → Release $test_name → Release $modifier_name"

    # TODO: Implement actual key injection
    # For now, just show what we would do
    echo "  → Press $modifier_key"
    echo "  → Press $test_key"
    echo "  → Release $test_key"
    echo "  → Release $modifier_key"
    echo ""
}

# Test M0 (B=48) + W=17, O=24, E=18, U=22
test_modal_combo 48 "B" 17 "W" "1"
test_modal_combo 48 "B" 24 "O" "2"
test_modal_combo 48 "B" 18 "E" "3"
test_modal_combo 48 "B" 22 "U" "4"

echo ""
echo "=========================================="
echo "Manual verification needed:"
echo "  1. Open investigate window in yamy-gui"
echo "  2. Hold B and press W,O,E,U"
echo "  3. Check if numbers 1,2,3,4 appear"
echo "=========================================="
