#!/bin/bash
# Visual Mapping Tester - Shows input → output in real-time

echo "╔════════════════════════════════════════════════════════════════════════════╗"
echo "║                    YAMY Visual Mapping Tester                              ║"
echo "╚════════════════════════════════════════════════════════════════════════════╝"
echo ""
echo "Testing letter mappings from config_clean.mayu..."
echo ""
echo "Press each key and see what YAMY outputs:"
echo ""
echo "┌────────────┬──────────────┬────────────────┬────────────┐"
echo "│ Press Key  │ Should Map   │ Scan Codes     │   Result   │"
echo "├────────────┼──────────────┼────────────────┼────────────┤"

# Test W-row (QWERTY top letter row)
test_key() {
    local key_name="$1"
    local input_code="$2"
    local expected_name="$3"
    local expected_code="$4"

    # Clear logs
    > /tmp/yamy_test.log

    # Inject key
    ../build/bin/yamy-test inject $input_code > /tmp/yamy_test.log 2>&1
    sleep 0.05

    # Check debug log for actual output
    local layer1=$(grep "\[LAYER1:IN\].*$input_code" /tmp/yamy_debug.log 2>/dev/null | tail -1)
    local layer2=$(grep "\[LAYER2:OUT\]" /tmp/yamy_debug.log 2>/dev/null | tail -1)
    local layer3=$(grep "\[LAYER3:OUT\].*evdev $expected_code" /tmp/yamy_debug.log 2>/dev/null | tail -1)

    if [ -n "$layer3" ]; then
        printf "│ %-10s │ %-12s │ %d → %d        │ %-10s │\n" "$key_name" "$expected_name" "$input_code" "$expected_code" "✓ PASS"
    else
        printf "│ %-10s │ %-12s │ %d → %d        │ %-10s │\n" "$key_name" "$expected_name" "$input_code" "$expected_code" "✗ FAIL"
    fi
}

# Test W-row (QWERTY positions Q W E R T Y U I O P)
test_key "Q" 16 "Minus" 12
test_key "W" 17 "A" 30
test_key "E" 18 "O" 24
test_key "R" 19 "E" 18
test_key "T" 20 "U" 22
test_key "Y" 21 "I" 23
test_key "U" 22 "D" 32
test_key "I" 23 "H" 35
test_key "O" 24 "T" 20
test_key "P" 25 "N" 49

echo "└────────────┴──────────────┴────────────────┴────────────┘"
echo ""
echo "If you see ✗ FAIL, the mapping is not working."
echo "If you see ✓ PASS, the mapping works correctly!"
echo ""

# Summary
passes=$(grep "✓ PASS" /tmp/yamy_visual_test.log 2>/dev/null | wc -l || echo "0")
fails=$(grep "✗ FAIL" /tmp/yamy_visual_test.log 2>/dev/null | wc -l || echo "0")

echo "Summary: $passes passed, $fails failed"
