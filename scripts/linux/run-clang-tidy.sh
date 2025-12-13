#!/bin/bash
# Run clang-tidy on C++ source files
# Usage: ./scripts/run-clang-tidy.sh [--fix] [file|directory...]
#
# Options:
#   --fix      Automatically apply suggested fixes
#   --help     Show this help message
#
# Examples:
#   ./scripts/run-clang-tidy.sh                        # Analyze platform/linux code
#   ./scripts/run-clang-tidy.sh src/platform/linux/    # Analyze specific directory
#   ./scripts/run-clang-tidy.sh --fix file.cpp         # Analyze and fix a file

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default options
FIX_MODE=false
TARGETS=()

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --fix)
            FIX_MODE=true
            shift
            ;;
        --help|-h)
            head -18 "$0" | tail -n +2 | sed 's/^# //' | sed 's/^#//'
            exit 0
            ;;
        *)
            TARGETS+=("$1")
            shift
            ;;
    esac
done

# Default to platform/linux if no targets specified
if [ ${#TARGETS[@]} -eq 0 ]; then
    TARGETS=("$PROJECT_ROOT/src/platform/linux")
fi

# Check if clang-tidy is available
if ! command -v clang-tidy &> /dev/null; then
    echo -e "${RED}Error: clang-tidy not found${NC}"
    echo "Install with: sudo apt-get install clang-tidy"
    exit 1
fi

# Find all C++ files
FILES=()
for TARGET in "${TARGETS[@]}"; do
    if [ -f "$TARGET" ]; then
        FILES+=("$TARGET")
    elif [ -d "$TARGET" ]; then
        while IFS= read -r -d '' file; do
            FILES+=("$file")
        done < <(find "$TARGET" \( -name '*.cpp' -o -name '*.h' \) -print0 2>/dev/null)
    else
        echo -e "${YELLOW}Warning: $TARGET not found${NC}"
    fi
done

if [ ${#FILES[@]} -eq 0 ]; then
    echo -e "${YELLOW}No C++ files found to analyze${NC}"
    exit 0
fi

echo "Analyzing ${#FILES[@]} files with clang-tidy..."
echo ""

# Build clang-tidy options
TIDY_OPTS="-p=$PROJECT_ROOT"
if $FIX_MODE; then
    TIDY_OPTS="$TIDY_OPTS --fix"
fi

# Track issues
ISSUES_FOUND=0

for FILE in "${FILES[@]}"; do
    echo -e "${YELLOW}Analyzing: $FILE${NC}"

    # Run clang-tidy with standard include paths
    OUTPUT=$(clang-tidy "$FILE" -- -std=c++17 -I"$PROJECT_ROOT/src" 2>&1) || true

    if echo "$OUTPUT" | grep -q "warning:\|error:"; then
        echo "$OUTPUT" | grep -E "warning:|error:" | head -20
        ((ISSUES_FOUND++))
    fi
    echo ""
done

# Summary
echo "----------------------------------------"
if [ $ISSUES_FOUND -gt 0 ]; then
    echo -e "${YELLOW}Files with issues: $ISSUES_FOUND${NC}"
    if ! $FIX_MODE; then
        echo "Run with --fix to automatically apply safe fixes."
    fi
else
    echo -e "${GREEN}No issues found in ${#FILES[@]} files.${NC}"
fi
