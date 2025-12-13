#!/usr/bin/env bash
set -euo pipefail

# Cross-compilation script for building Windows binaries on Linux using MinGW

CLEAN=false
if [[ "${1:-}" == "--clean" ]]; then
    CLEAN=true
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DIST_DIR="$ROOT/dist"
RELEASE_DIR="$ROOT/dist/yamy-release"
LOGS_DIR="$ROOT/logs"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo_color() {
    local color=$1
    shift
    echo -e "${color}$*${NC}"
}

# Check for MinGW toolchains
check_dependencies() {
    echo_color "$CYAN" "Checking dependencies..."

    local missing=()

    if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
        missing+=("mingw-w64 64-bit toolchain (x86_64-w64-mingw32-gcc)")
    fi

    if ! command -v i686-w64-mingw32-gcc &> /dev/null; then
        missing+=("mingw-w64 32-bit toolchain (i686-w64-mingw32-gcc)")
    fi

    if ! command -v cmake &> /dev/null; then
        missing+=("cmake")
    fi

    if [[ ${#missing[@]} -gt 0 ]]; then
        echo_color "$RED" "Missing dependencies:"
        for dep in "${missing[@]}"; do
            echo_color "$RED" "  - $dep"
        done
        echo ""
        echo_color "$YELLOW" "Install on Ubuntu/Debian:"
        echo "  sudo apt-get install mingw-w64 cmake"
        echo ""
        echo_color "$YELLOW" "Install on Arch Linux:"
        echo "  sudo pacman -S mingw-w64-gcc cmake"
        echo ""
        echo_color "$YELLOW" "Install on Fedora:"
        echo "  sudo dnf install mingw64-gcc mingw32-gcc cmake"
        exit 1
    fi

    echo_color "$GREEN" "All dependencies found."
}

# Clean
if [[ "$CLEAN" == true ]] && [[ -d "$ROOT/build" ]]; then
    echo_color "$YELLOW" "Cleaning build directory..."
    rm -rf "$ROOT/build"
fi

if [[ -d "$DIST_DIR" ]]; then
    rm -rf "$DIST_DIR"
fi

mkdir -p "$RELEASE_DIR"
mkdir -p "$LOGS_DIR"

# Check dependencies
check_dependencies

# -----------------------------------------------------------------------------
# Quality Checks (temporarily disabled for debugging)
# -----------------------------------------------------------------------------
# echo_color "$CYAN" "Running Anti-Pattern Check..."
# bash "$SCRIPT_DIR/check_antipatterns.sh" || echo_color "$YELLOW" "Anti-pattern check failed, continuing anyway..."

# echo_color "$CYAN" "Running Missing Sources Check..."
# bash "$SCRIPT_DIR/check_missing_sources.sh" || echo_color "$YELLOW" "Missing sources check failed, continuing anyway..."

# echo_color "$CYAN" "Running Encoding Check..."
# bash "$SCRIPT_DIR/check_encoding.sh" || echo_color "$YELLOW" "Encoding check failed, continuing anyway..."

echo_color "$YELLOW" "Quality checks disabled for this build"

# -----------------------------------------------------------------------------
# Create CMake Toolchain Files
# -----------------------------------------------------------------------------
mkdir -p "$ROOT/build/toolchain"

cat > "$ROOT/build/toolchain/mingw-w64-x86_64.cmake" << 'EOF'
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF

cat > "$ROOT/build/toolchain/mingw-w64-i686.cmake" << 'EOF'
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF

# -----------------------------------------------------------------------------
# Build 64-bit
# -----------------------------------------------------------------------------
echo_color "$CYAN" "Building 64-bit..."
cmake -S "$ROOT" -B "$ROOT/build/x64" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/build/toolchain/mingw-w64-x86_64.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
    -DCMAKE_SHARED_LINKER_FLAGS="-static-libgcc -static-libstdc++"

cmake --build "$ROOT/build/x64" --config Release > "$LOGS_DIR/build_log_x64.txt" 2>&1 || {
    echo_color "$RED" "CMake Build 64-bit failed. Check $LOGS_DIR/build_log_x64.txt"
    exit 1
}

# Check dependencies for missing DLLs
check_deps() {
    local file=$1
    local arch=$2
    echo_color "$YELLOW" "  Checking: $(basename $file)"

    # Select the correct objdump based on architecture
    local objdump_cmd
    if [[ "$arch" == "x64" ]]; then
        objdump_cmd="x86_64-w64-mingw32-objdump"
    else
        objdump_cmd="i686-w64-mingw32-objdump"
    fi

    # Get DLL dependencies
    local deps=$($objdump_cmd -p "$file" 2>/dev/null | grep "DLL Name:" || true)

    if echo "$deps" | grep -qi "libstdc++\|libgcc\|libwinpthread"; then
        echo_color "$RED" "  ERROR: $file depends on MinGW runtime DLLs!"
        echo "$deps" | grep -i "DLL Name:"
        echo_color "$RED" "  These DLLs must be statically linked or bundled."
        return 1
    fi

    echo_color "$GREEN" "  ✓ No MinGW runtime dependencies"
    return 0
}

echo_color "$CYAN" "Checking 64-bit dependencies..."

if ! check_deps "$ROOT/build/x64/bin/yamy64.exe" "x64"; then
    echo_color "$RED" "Dependency check failed. Aborting."
    exit 1
fi

# Copy 64-bit artifacts
echo_color "$CYAN" "Copying 64-bit artifacts..."
cp "$ROOT/build/x64/bin/yamy.exe" "$RELEASE_DIR/"
cp "$ROOT/build/x64/bin/yamy64.exe" "$RELEASE_DIR/"
cp "$ROOT/build/x64/bin/yamy64.dll" "$RELEASE_DIR/"

# -----------------------------------------------------------------------------
# Build 32-bit
# -----------------------------------------------------------------------------
echo_color "$CYAN" "Building 32-bit..."
cmake -S "$ROOT" -B "$ROOT/build/x86" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/build/toolchain/mingw-w64-i686.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
    -DCMAKE_SHARED_LINKER_FLAGS="-static-libgcc -static-libstdc++"

cmake --build "$ROOT/build/x86" --config Release > "$LOGS_DIR/build_log_x86.txt" 2>&1 || {
    echo_color "$RED" "CMake Build 32-bit failed. Check $LOGS_DIR/build_log_x86.txt"
    exit 1
}

# Check 32-bit dependencies
echo_color "$CYAN" "Checking 32-bit dependencies..."
if ! check_deps "$ROOT/build/x86/bin/yamy32.exe" "x86"; then
    echo_color "$RED" "32-bit dependency check failed. Aborting."
    exit 1
fi

# Copy 32-bit artifacts
echo_color "$CYAN" "Copying 32-bit artifacts..."
cp "$ROOT/build/x86/bin/yamy32.exe" "$RELEASE_DIR/"
cp "$ROOT/build/x86/bin/yamy32.dll" "$RELEASE_DIR/"
cp "$ROOT/build/x86/bin/yamyd32.exe" "$RELEASE_DIR/"

# -----------------------------------------------------------------------------
# Copy Assets (Keymaps, Docs, Scripts)
# -----------------------------------------------------------------------------
echo_color "$CYAN" "Copying Assets..."

cp -r "$ROOT/keymaps" "$RELEASE_DIR/"
# Copy only launch scripts to root
cp "$ROOT/scripts/windows/launch_yamy.bat" "$RELEASE_DIR/"
cp "$ROOT/scripts/windows/launch_yamy_admin.bat" "$RELEASE_DIR/"
cp -r "$ROOT/docs" "$RELEASE_DIR/" || true
cp "$ROOT/docs/readme.txt" "$RELEASE_DIR/readme.txt" || true

# -----------------------------------------------------------------------------
# Create Zip
# -----------------------------------------------------------------------------
echo_color "$CYAN" "Creating Archive..."
ZIP_PATH="$DIST_DIR/yamy-dist.zip"
(cd "$RELEASE_DIR" && zip -r "$ZIP_PATH" .)

# Cleanup release dir safely (keeping zip)
rm -rf "$RELEASE_DIR"

echo_color "$GREEN" "Success! Created $ZIP_PATH"
