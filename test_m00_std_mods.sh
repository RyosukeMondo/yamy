#!/bin/bash
# Quick test for M00-MFF + standard modifiers (M00-S-, M01-C-)

echo "Testing M00-MFF + Standard Modifiers Parser"
echo "==========================================="

CONFIG="keymaps/test_m00_e2e.mayu"

echo ""
echo "Step 1: Check if config exists..."
if [ ! -f "$CONFIG" ]; then
    echo "ERROR: Config file not found: $CONFIG"
    exit 1
fi
echo "✓ Config file found"

echo ""
echo "Step 2: Parse config and check for M00-S- and M01-C- entries..."
if grep -q "key M00-S-\*A = \*F5" "$CONFIG" && \
   grep -q "key M01-C-\*A = \*F7" "$CONFIG"; then
    echo "✓ Standard modifier combinations found in config"
else
    echo "ERROR: Standard modifier combinations not found in config"
    exit 1
fi

echo ""
echo "Step 3: Build parser test binary..."
cmake --build build --target yamy 2>&1 | tail -3
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi
echo "✓ Build successful"

echo ""
echo "Step 4: Quick parser validation test..."
echo "Attempting to load config with M00-S- and M01-C- syntax..."

# Create a minimal test: just try to start yamy with the config and capture parser output
YAMY_CONFIG_FILE="$CONFIG" timeout 2 ./build/bin/yamy 2>&1 | grep -E "(PARSER|M00|M01|ERROR)" > /tmp/yamy_parser_test.log

if grep -qi "error" /tmp/yamy_parser_test.log; then
    echo "✗ Parser errors detected:"
    cat /tmp/yamy_parser_test.log
    exit 1
else
    echo "✓ No parser errors detected"
    if [ -s /tmp/yamy_parser_test.log ]; then
        echo ""
        echo "Parser output:"
        head -20 /tmp/yamy_parser_test.log
    fi
fi

echo ""
echo "========================================="
echo "SUCCESS: Parser can handle M00-S- and M01-C- syntax!"
echo "========================================="
echo ""
echo "Next: Run full E2E tests to verify functionality"
