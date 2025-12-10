#!/bin/bash
# Format C++ source code using clang-format
# Usage: ./scripts/format-code.sh [--check] [file|directory...]
#
# Options:
#   --check    Check formatting without modifying files (exit 1 if issues found)
#   --help     Show this help message
#
# Examples:
#   ./scripts/format-code.sh                    # Format all files in src/
#   ./scripts/format-code.sh src/platform/      # Format specific directory
#   ./scripts/format-code.sh --check            # Check all files without modifying
#   ./scripts/format-code.sh file.cpp           # Format a single file

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default options
CHECK_ONLY=false
TARGETS=()

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --check)
            CHECK_ONLY=true
            shift
            ;;
        --help|-h)
            head -20 "$0" | tail -n +2 | sed 's/^# //' | sed 's/^#//'
            exit 0
            ;;
        *)
            TARGETS+=("$1")
            shift
            ;;
    esac
done

# Default to src/ if no targets specified
if [ ${#TARGETS[@]} -eq 0 ]; then
    TARGETS=("$PROJECT_ROOT/src")
fi

# Check if clang-format is available
if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}Error: clang-format not found${NC}"
    echo "Install with: sudo apt-get install clang-format"
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
        done < <(find "$TARGET" \( -name '*.cpp' -o -name '*.h' \) -print0 | grep -zv '/third_party/' | grep -zv '/external/')
    else
        echo -e "${YELLOW}Warning: $TARGET not found${NC}"
    fi
done

if [ ${#FILES[@]} -eq 0 ]; then
    echo -e "${YELLOW}No C++ files found to format${NC}"
    exit 0
fi

echo "Found ${#FILES[@]} files to process..."

# Process files
UNFORMATTED=()
FORMATTED=0

for FILE in "${FILES[@]}"; do
    if $CHECK_ONLY; then
        if ! clang-format --dry-run --Werror "$FILE" 2>/dev/null; then
            UNFORMATTED+=("$FILE")
        fi
    else
        clang-format -i "$FILE"
        ((FORMATTED++))
    fi
done

# Report results
if $CHECK_ONLY; then
    if [ ${#UNFORMATTED[@]} -gt 0 ]; then
        echo -e "${RED}The following files need formatting:${NC}"
        for FILE in "${UNFORMATTED[@]}"; do
            echo "  - $FILE"
        done
        echo ""
        echo "Run './scripts/format-code.sh' to fix formatting issues."
        exit 1
    else
        echo -e "${GREEN}All ${#FILES[@]} files are properly formatted.${NC}"
    fi
else
    echo -e "${GREEN}Formatted $FORMATTED files.${NC}"
fi
