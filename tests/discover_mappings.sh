#!/bin/bash
# Discovery tool: Find out what each key actually maps to with config.mayu

BUILD_DIR="../build"
KEYMAP_DIR="../keymaps"
YAMY_TEST="$BUILD_DIR/bin/yamy-test"

# Create config.mayu only configuration
echo 'include "109.mayu"
include "key_seq.mayu"
include "config.mayu"' > "$KEYMAP_DIR/master.mayu"

echo "=========================================="
echo "YAMY Mapping Discovery Tool"
echo "Testing what keys actually produce"
echo "=========================================="
echo ""

# Test common keys
test_keys=(
    "30:A"
    "48:B"
    "46:C"
    "47:V"
    "17:W"
    "19:R"
    "16:Q"
    "15:TAB"
    "28:ENTER"
)

for entry in "${test_keys[@]}"; do
    IFS=':' read -r keycode name <<< "$entry"
    echo -n "Testing $name (code $keycode): "

    # Run e2e-auto and capture result
    result=$($YAMY_TEST e2e-auto $keycode $keycode 2>&1 | grep "Captured:")

    if [ -n "$result" ]; then
        # Extract captured key code
        captured=$(echo "$result" | sed 's/Captured: //' | awk '{print $1}')
        keyname=$(echo "$result" | sed 's/.*(//' | sed 's/).*//')
        echo "$name → $captured ($keyname)"
    else
        echo "$name → (no output captured)"
    fi

    sleep 1
done

# Restore original
git checkout "$KEYMAP_DIR/master.mayu" 2>/dev/null || true

echo ""
echo "Discovery complete!"
