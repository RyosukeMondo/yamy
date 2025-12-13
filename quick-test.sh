#!/bin/bash
# Quick automated test of yamy IPC functionality

set -e

YAMY_BIN="./bin/yamy"
YAMY_CTL="./bin/yamy-ctl"

echo "🧪 YAMY Quick Test"
echo "=================="
echo ""

# Cleanup any existing processes
pkill -9 yamy 2>/dev/null || true
sleep 1
rm -f /tmp/yamy-engine.sock

# Check if we're in build directory
if [ ! -f "$YAMY_BIN" ]; then
    echo "❌ Error: Not in build directory. Run from build_release/"
    exit 1
fi

echo "1. Starting yamy..."
$YAMY_BIN --no-restore > /tmp/yamy-quick-test.log 2>&1 &
YAMY_PID=$!
sleep 3

if ! ps -p $YAMY_PID > /dev/null; then
    echo "❌ Error: Yamy failed to start"
    cat /tmp/yamy-quick-test.log
    exit 1
fi
echo "✅ Yamy started (PID: $YAMY_PID)"

echo ""
echo "2. Testing IPC commands..."

echo "   - status:"
$YAMY_CTL status
echo ""

echo "   - config:"
$YAMY_CTL config
echo ""

echo "   - keymaps:"
$YAMY_CTL keymaps
echo ""

echo "   - metrics:"
$YAMY_CTL metrics
echo ""

echo "3. Creating test config..."
cat > /tmp/test-yamy.mayu <<'EOF'
# Minimal test configuration
keymap Global
    key CapsLock = Escape
EOF

echo "4. Loading test config..."
$YAMY_CTL reload /tmp/test-yamy.mayu
echo ""

echo "5. Checking config loaded..."
$YAMY_CTL config | grep "test-yamy"
if [ $? -eq 0 ]; then
    echo "✅ Config loaded successfully"
else
    echo "❌ Config load failed"
fi
echo ""

echo "6. Starting engine..."
$YAMY_CTL start
sleep 1
echo ""

echo "7. Checking engine status..."
$YAMY_CTL status | grep -i "running"
if [ $? -eq 0 ]; then
    echo "✅ Engine is running"
else
    echo "⚠️  Engine might not be running (check status above)"
fi
echo ""

echo "8. Stopping engine..."
$YAMY_CTL stop
sleep 1
echo ""

echo "9. Cleaning up..."
pkill yamy 2>/dev/null
sleep 1
echo ""

echo "=================="
echo "✅ Quick test complete!"
echo ""
echo "Next steps:"
echo "  - Check system tray for yamy icon"
echo "  - Test keyboard remapping (press CapsLock, should act as Escape)"
echo "  - See MANUAL-TEST-GUIDE.md for detailed testing"
echo ""
echo "Debug log: /tmp/yamy-quick-test.log"
echo "=================="
