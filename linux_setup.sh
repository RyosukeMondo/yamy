#!/bin/bash
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# linux_setup.sh - Setup script for YAMY Linux input capture
#
# This script configures permissions for evdev input capture without root.
# Run once after installation.
#
# Usage: sudo ./linux_setup.sh [username]
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set -e  # Exit on error

SCRIPT_NAME="$(basename "$0")"
COLOR_RED='\033[0;31m'
COLOR_GREEN='\033[0;32m'
COLOR_YELLOW='\033[1;33m'
COLOR_BLUE='\033[0;34m'
COLOR_RESET='\033[0m'

# Detect target user
if [ -n "$1" ]; then
    TARGET_USER="$1"
elif [ -n "$SUDO_USER" ]; then
    TARGET_USER="$SUDO_USER"
else
    TARGET_USER="$USER"
fi

echo -e "${COLOR_BLUE}========================================${COLOR_RESET}"
echo -e "${COLOR_BLUE}  YAMY Linux Input Setup${COLOR_RESET}"
echo -e "${COLOR_BLUE}========================================${COLOR_RESET}"
echo ""
echo "Target user: $TARGET_USER"
echo ""

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Check if running as root
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
if [ "$EUID" -ne 0 ]; then
    echo -e "${COLOR_RED}ERROR: This script must be run with sudo${COLOR_RESET}"
    echo "Usage: sudo ./$SCRIPT_NAME [username]"
    exit 1
fi

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Verify user exists
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
if ! id "$TARGET_USER" &>/dev/null; then
    echo -e "${COLOR_RED}ERROR: User '$TARGET_USER' does not exist${COLOR_RESET}"
    exit 1
fi

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Step 1: Add user to input group
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
echo -e "${COLOR_YELLOW}[1/5] Adding user to 'input' group...${COLOR_RESET}"

if groups "$TARGET_USER" | grep -q "\binput\b"; then
    echo -e "${COLOR_GREEN}  ✓ User already in 'input' group${COLOR_RESET}"
else
    usermod -a -G input "$TARGET_USER"
    echo -e "${COLOR_GREEN}  ✓ User added to 'input' group${COLOR_RESET}"
    NEEDS_RELOGIN=1
fi

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Step 2: Verify /dev/input exists
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
echo -e "${COLOR_YELLOW}[2/5] Checking /dev/input...${COLOR_RESET}"

if [ ! -d "/dev/input" ]; then
    echo -e "${COLOR_RED}  ✗ /dev/input does not exist${COLOR_RESET}"
    echo -e "${COLOR_RED}    Your system may not support evdev${COLOR_RESET}"
    exit 1
fi

EVENT_COUNT=$(ls /dev/input/event* 2>/dev/null | wc -l)
if [ "$EVENT_COUNT" -eq 0 ]; then
    echo -e "${COLOR_RED}  ✗ No input devices found in /dev/input/${COLOR_RESET}"
    exit 1
fi

echo -e "${COLOR_GREEN}  ✓ Found $EVENT_COUNT input device(s)${COLOR_RESET}"

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Step 3: Check udev rules (optional enhancement)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
echo -e "${COLOR_YELLOW}[3/5] Checking udev rules...${COLOR_RESET}"

UDEV_RULE="/etc/udev/rules.d/99-input.rules"
if [ ! -f "$UDEV_RULE" ]; then
    echo -e "${COLOR_BLUE}  Creating udev rule for input group...${COLOR_RESET}"
    cat > "$UDEV_RULE" <<'EOF'
# Allow input group to access input devices
KERNEL=="event*", SUBSYSTEM=="input", MODE="0660", GROUP="input"
EOF
    udevadm control --reload-rules
    udevadm trigger
    echo -e "${COLOR_GREEN}  ✓ Created $UDEV_RULE${COLOR_RESET}"
else
    echo -e "${COLOR_GREEN}  ✓ udev rules already exist${COLOR_RESET}"
fi

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Step 4: Load uinput kernel module
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
echo -e "${COLOR_YELLOW}[4/5] Checking uinput kernel module...${COLOR_RESET}"

if lsmod | grep -q "^uinput"; then
    echo -e "${COLOR_GREEN}  ✓ uinput module already loaded${COLOR_RESET}"
else
    modprobe uinput
    echo -e "${COLOR_GREEN}  ✓ Loaded uinput module${COLOR_RESET}"
fi

# Make uinput load on boot
if [ ! -f "/etc/modules-load.d/uinput.conf" ]; then
    echo "uinput" > /etc/modules-load.d/uinput.conf
    echo -e "${COLOR_GREEN}  ✓ uinput will load on boot${COLOR_RESET}"
fi

# Set uinput permissions
if [ -c "/dev/uinput" ]; then
    chgrp input /dev/uinput
    chmod 660 /dev/uinput
    echo -e "${COLOR_GREEN}  ✓ Set /dev/uinput permissions${COLOR_RESET}"
fi

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Step 5: Verify permissions
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
echo -e "${COLOR_YELLOW}[5/5] Verifying permissions...${COLOR_RESET}"

# Check if input group can access devices
FIRST_EVENT=$(ls /dev/input/event* 2>/dev/null | head -1)
if [ -n "$FIRST_EVENT" ]; then
    EVENT_GROUP=$(stat -c '%G' "$FIRST_EVENT")
    EVENT_PERMS=$(stat -c '%a' "$FIRST_EVENT")

    echo "  Device: $FIRST_EVENT"
    echo "  Group:  $EVENT_GROUP"
    echo "  Perms:  $EVENT_PERMS"

    if [ "$EVENT_GROUP" = "input" ] && [ "${EVENT_PERMS:1:1}" -ge 6 ]; then
        echo -e "${COLOR_GREEN}  ✓ Permissions correct${COLOR_RESET}"
    else
        echo -e "${COLOR_YELLOW}  ! Permissions may need adjustment${COLOR_RESET}"
    fi
fi

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Final Summary
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
echo ""
echo -e "${COLOR_BLUE}========================================${COLOR_RESET}"
echo -e "${COLOR_GREEN}  Setup Complete!${COLOR_RESET}"
echo -e "${COLOR_BLUE}========================================${COLOR_RESET}"
echo ""

if [ -n "$NEEDS_RELOGIN" ]; then
    echo -e "${COLOR_YELLOW}⚠  IMPORTANT: Log out and log back in for group changes to take effect${COLOR_RESET}"
    echo ""
    echo "After re-login, verify with:"
    echo "  groups | grep input"
    echo ""
else
    echo "You can now run YAMY."
fi

echo "Test input access:"
echo "  ls -la /dev/input/event*"
echo "  groups \$USER | grep input"
echo ""

exit 0
