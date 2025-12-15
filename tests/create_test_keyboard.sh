#!/bin/bash
# Creates a persistent test keyboard device for YAMY testing
# Usage: ./create_test_keyboard.sh [device_name]

DEVICE_NAME=${1:-"YAMY-Test-Keyboard"}
FIFO="/tmp/yamy_test_keyboard_${$}.fifo"

# Cleanup on exit
cleanup() {
    rm -f "$FIFO"
    exit 0
}
trap cleanup EXIT INT TERM

# Create named pipe for commands
mkfifo "$FIFO" 2>/dev/null || true

echo "Creating test keyboard device: $DEVICE_NAME"
echo "Command pipe: $FIFO"
echo "Send key codes to $FIFO (format: <evdev_code> <press|release>)"
echo "Send 'quit' to exit"

# Keep device alive and process commands
exec 3< "$FIFO"
build/bin/yamy-inject --daemon --name "$DEVICE_NAME" --pipe "$FIFO" &
INJECT_PID=$!

# Wait for commands
while read -u 3 cmd; do
    if [ "$cmd" = "quit" ]; then
        break
    fi
done

kill $INJECT_PID 2>/dev/null
wait $INJECT_PID 2>/dev/null
