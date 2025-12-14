#!/usr/bin/env bash
set -euo pipefail

# Fast install helper for mold + LLD on common Linux distros.
# Usage: ./scripts/linux/install_mold.sh

install_apt() {
    sudo apt-get update
    sudo apt-get install -y mold lld
}

install_dnf() {
    sudo dnf install -y mold lld
}

install_pacman() {
    sudo pacman -Sy --noconfirm mold lld
}

if command -v apt-get >/dev/null 2>&1; then
    install_apt
elif command -v dnf >/dev/null 2>&1; then
    install_dnf
elif command -v pacman >/dev/null 2>&1; then
    install_pacman
else
    echo "Package manager not detected. Install mold + LLD manually from https://github.com/rui314/mold/releases"
    exit 1
fi

echo "mold + LLD installation complete."
