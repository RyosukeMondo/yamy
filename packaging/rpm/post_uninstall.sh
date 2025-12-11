#!/bin/sh
# YAMY post-uninstall script for RPM-based distributions (Fedora/openSUSE)
# This script runs after package removal to clean up

UDEV_RULES_FILE="/etc/udev/rules.d/99-yamy-input.rules"

# Only run on complete uninstall (not upgrade)
# $1 == 0 means complete removal, $1 == 1 means upgrade
if [ "$1" = "0" ]; then
    # Remove udev rules
    if [ -f "${UDEV_RULES_FILE}" ]; then
        rm -f "${UDEV_RULES_FILE}"
        # Reload udev rules
        if command -v udevadm >/dev/null 2>&1; then
            udevadm control --reload-rules 2>/dev/null || true
        fi
    fi
fi

exit 0
