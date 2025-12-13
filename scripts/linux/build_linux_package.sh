#!/usr/bin/env bash
set -euo pipefail

# Linux packaging script for YAMY

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DIST_DIR="$ROOT/dist"
VERSION="${1:-1.0.1}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo_color() {
    local color=$1
    shift
    echo -e "${color}$*${NC}"
}

echo_color "$CYAN" "Building YAMY Linux Package v${VERSION}"

# Clean previous builds
if [[ -d "$ROOT/build_release" ]]; then
    echo_color "$YELLOW" "Cleaning previous build..."
    rm -rf "$ROOT/build_release"
fi

if [[ -d "$DIST_DIR" ]]; then
    rm -rf "$DIST_DIR"
fi

mkdir -p "$DIST_DIR"

# Configure CMake
echo_color "$CYAN" "Configuring CMake..."
cmake -B "$ROOT/build_release" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_LINUX_STUB=ON \
    -DBUILD_QT_GUI=ON \
    -DBUILD_LINUX_TESTING=OFF \
    -DBUILD_REGRESSION_TESTS=OFF \
    -DCMAKE_INSTALL_PREFIX=/usr

# Build
echo_color "$CYAN" "Building..."
cmake --build "$ROOT/build_release" -j"$(nproc)"

# Create tarball
echo_color "$CYAN" "Creating tarball..."
TARBALL_DIR="$DIST_DIR/yamy-${VERSION}-linux-x86_64"
mkdir -p "$TARBALL_DIR/bin"
mkdir -p "$TARBALL_DIR/share/yamy"
mkdir -p "$TARBALL_DIR/share/doc/yamy"

# Copy binaries
if [[ -f "$ROOT/build_release/bin/yamy" ]]; then
    cp "$ROOT/build_release/bin/yamy" "$TARBALL_DIR/bin/"
fi

if [[ -f "$ROOT/build_release/bin/yamy-ctl" ]]; then
    cp "$ROOT/build_release/bin/yamy-ctl" "$TARBALL_DIR/bin/"
fi

# Copy keymaps and documentation
cp -r "$ROOT/keymaps" "$TARBALL_DIR/share/yamy/" || true
cp "$ROOT/README.md" "$TARBALL_DIR/share/doc/yamy/" || true
cp "$ROOT/LICENSE" "$TARBALL_DIR/share/doc/yamy/" || true
cp "$ROOT/CHANGELOG.md" "$TARBALL_DIR/share/doc/yamy/" || true

# Create tarball
cd "$DIST_DIR"
tar -czf "yamy-${VERSION}-linux-x86_64.tar.gz" "yamy-${VERSION}-linux-x86_64"
rm -rf "yamy-${VERSION}-linux-x86_64"

echo_color "$GREEN" "Success! Created:"
echo_color "$GREEN" "  $DIST_DIR/yamy-${VERSION}-linux-x86_64.tar.gz"

# List what was built
echo_color "$CYAN" "\nBuilt binaries:"
ls -lh "$ROOT/build_release/bin/" || true
