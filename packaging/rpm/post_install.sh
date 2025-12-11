#!/bin/sh
# YAMY post-installation script for RPM-based distributions (Fedora/openSUSE)
# This script runs after package installation to:
# 1. Set up udev rules for input device access
# 2. Notify the user about 'input' group membership requirements

UDEV_RULES_FILE="/etc/udev/rules.d/99-yamy-input.rules"

# Create udev rules for input device access
# This allows YAMY to read from /dev/input/event* devices
cat > "${UDEV_RULES_FILE}" << 'EOF'
# YAMY keyboard remapper - input device access rules
# This allows members of the 'input' group to access keyboard devices

# Grant read access to input event devices for 'input' group
KERNEL=="event*", SUBSYSTEM=="input", MODE="0660", GROUP="input"

# Grant read access to keyboard devices specifically
SUBSYSTEM=="input", ATTRS{name}=="*keyboard*", MODE="0660", GROUP="input"
SUBSYSTEM=="input", ATTRS{name}=="*Keyboard*", MODE="0660", GROUP="input"
EOF

# Reload udev rules
if command -v udevadm >/dev/null 2>&1; then
    udevadm control --reload-rules 2>/dev/null || true
    udevadm trigger 2>/dev/null || true
fi

# Check if input group exists, create if needed
if ! getent group input >/dev/null 2>&1; then
    groupadd -r input || true
fi

# Display post-install message
echo ""
echo "============================================================"
echo "YAMY has been installed successfully!"
echo ""
echo "IMPORTANT: To use YAMY keyboard remapping, your user must"
echo "be a member of the 'input' group."
echo ""
echo "Run: sudo usermod -aG input \$USER"
echo ""
echo "Then log out and log back in for changes to take effect."
echo "============================================================"
echo ""

exit 0
