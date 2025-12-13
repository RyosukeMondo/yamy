#!/bin/bash
# Investigation Tool: Analyze keycode transformations through all layers
# Shows what happens at each layer: Input → YAMY → Subst → Output

BUILD_DIR="../build"
KEYMAP_DIR="../keymaps"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=========================================="
echo "YAMY Keycode Layer Investigation Tool"
echo "=========================================="
echo ""
echo "This tool helps investigate what happens to keycodes"
echo "as they flow through YAMY's transformation layers:"
echo ""
echo -e "${CYAN}LAYER 1:${NC} evdev input → YAMY scan code (evdevToYamyKeyCode)"
echo -e "${CYAN}LAYER 2:${NC} YAMY scan code → def subst transformation (engine)"
echo -e "${CYAN}LAYER 3:${NC} YAMY scan code → evdev output (yamyToEvdevKeyCode)"
echo ""

# Check if config specified
CONFIG="${1:-config.mayu}"
TEST_KEY="${2:-30}"
TEST_KEY_NAME="${3:-A}"

echo "Configuration: $CONFIG"
echo "Test key: $TEST_KEY ($TEST_KEY_NAME)"
echo ""

# Create test configuration
cat > "$KEYMAP_DIR/master.mayu" << EOF
include "109.mayu"
include "key_seq.mayu"
include "$CONFIG"
EOF

echo "Created master.mayu:"
cat "$KEYMAP_DIR/master.mayu"
echo ""

# Kill any running YAMY
killall -9 yamy 2>/dev/null || true
sleep 1

echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Starting YAMY with debug logging...${NC}"
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Start YAMY with debug logging enabled
export YAMY_DEBUG_KEYCODE=1
$BUILD_DIR/bin/yamy > /tmp/yamy_debug.log 2>&1 &
YAMY_PID=$!

echo "YAMY PID: $YAMY_PID"
sleep 3

# Start engine
$BUILD_DIR/bin/yamy-ctl start
sleep 1

# Check status
echo ""
$BUILD_DIR/bin/yamy-ctl status
echo ""

echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Injecting test key: $TEST_KEY ($TEST_KEY_NAME)${NC}"
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Inject test key and capture output
$BUILD_DIR/bin/yamy-test e2e-auto $TEST_KEY $TEST_KEY 2>&1 | tail -15

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Debug Log Analysis (LAYER transformations)${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Extract layer transformation logs
echo -e "${CYAN}LAYER 1 (Input):${NC}"
grep "\[LAYER1:IN\]" /tmp/yamy_debug.log | tail -5
echo ""

echo -e "${CYAN}LAYER 3 (Output):${NC}"
grep "\[LAYER3:OUT\]" /tmp/yamy_debug.log | tail -10
echo ""

echo -e "${CYAN}Layout Detection:${NC}"
grep "layout" /tmp/yamy_debug.log | tail -3
echo ""

echo ""
echo "Full debug log available at: /tmp/yamy_debug.log"
echo ""

# Cleanup
killall -9 yamy 2>/dev/null || true

echo -e "${GREEN}Investigation complete!${NC}"
echo ""
echo "To investigate another key:"
echo "  $0 <config.mayu> <keycode> <keyname>"
echo ""
echo "Examples:"
echo "  $0 config.mayu 30 A"
echo "  $0 config.mayu 48 B"
echo "  $0 config.mayu 15 TAB"
