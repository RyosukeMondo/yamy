#!/usr/bin/env bash
#
# Pre-commit hook for code metrics enforcement
# Checks staged C++ files with lizard for:
# - Max 500 lines per file
# - Max 50 lines per function
# - Cyclomatic complexity ≤ 15
#
# To bypass: git commit --no-verify

set -e

# Color output
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Metrics limits (must match CMakeLists.txt configuration)
MAX_LINES_FILE=500
MAX_LINES_FUNCTION=50
MAX_CCN=15

# Check if lizard is installed
if ! command -v lizard &> /dev/null; then
    echo -e "${YELLOW}Warning: lizard not found. Skipping code metrics check.${NC}"
    echo "Install with: pip install lizard"
    exit 0
fi

# Get list of staged C++ files
STAGED_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|cc|cxx|c\+\+|h|hpp|hxx)$' || true)

if [ -z "$STAGED_FILES" ]; then
    # No C++ files staged, nothing to check
    exit 0
fi

echo "Running code metrics check on staged files..."
echo ""

# Create temporary file to store violations
VIOLATIONS_FILE=$(mktemp)
trap "rm -f $VIOLATIONS_FILE" EXIT

# Track if we found any violations
HAS_VIOLATIONS=0

# Process each staged file
while IFS= read -r file; do
    # Skip files that don't exist (e.g., deleted files)
    if [ ! -f "$file" ]; then
        continue
    fi

    # Run lizard on the file
    # --length: include function length
    # --CCN: include cyclomatic complexity
    # --warnings_only: only show violations
    # -w: enable warnings mode
    OUTPUT=$(lizard --length $MAX_LINES_FUNCTION --CCN $MAX_CCN -w "$file" 2>&1 || true)

    # Check if there are any warnings
    if echo "$OUTPUT" | grep -q "NLOC.*CCN"; then
        # Filter for actual violations (lines with high CCN or length)
        VIOLATIONS=$(echo "$OUTPUT" | awk -v max_len=$MAX_LINES_FUNCTION -v max_ccn=$MAX_CCN '
            /^[0-9]/ {
                # Parse lizard output format: NLOC CCN token PARAM length@nloc:nlin-nlin@file
                nloc = $1
                ccn = $2

                # Extract function info (everything after the numbers)
                func_info = ""
                for (i=6; i<=NF; i++) func_info = func_info $i " "

                # Check if violations exist
                if (ccn > max_ccn || nloc > max_len) {
                    print "  Line " $5 ": " func_info
                    if (nloc > max_len) print "    ⚠ Function length: " nloc " (max: " max_len ")"
                    if (ccn > max_ccn) print "    ⚠ Cyclomatic complexity: " ccn " (max: " max_ccn ")"
                }
            }
        ')

        if [ -n "$VIOLATIONS" ]; then
            echo "$file" >> "$VIOLATIONS_FILE"
            echo "$VIOLATIONS" >> "$VIOLATIONS_FILE"
            echo "" >> "$VIOLATIONS_FILE"
            HAS_VIOLATIONS=1
        fi
    fi

    # Also check file length
    # Use wc to count total lines, trim whitespace
    LINE_COUNT=$(wc -l < "$file" 2>/dev/null | tr -d ' ' || echo 0)
    # Count blank and comment lines
    BLANK_LINES=$(grep -cE '^\s*$|^\s*//' "$file" 2>/dev/null | tr -d ' ' || echo 0)
    # Calculate non-blank, non-comment lines
    NLOC=$((LINE_COUNT - BLANK_LINES))

    if [ "$NLOC" -gt "$MAX_LINES_FILE" ]; then
        echo "$file" >> "$VIOLATIONS_FILE"
        echo "  ⚠ File too long: $NLOC non-blank lines (max: $MAX_LINES_FILE)" >> "$VIOLATIONS_FILE"
        echo "" >> "$VIOLATIONS_FILE"
        HAS_VIOLATIONS=1
    fi

done <<< "$STAGED_FILES"

# Report results
if [ $HAS_VIOLATIONS -eq 1 ]; then
    echo -e "${RED}Code metrics violations found:${NC}"
    echo ""
    cat "$VIOLATIONS_FILE"
    echo -e "${RED}Commit rejected.${NC}"
    echo ""
    echo "Please fix the violations or use 'git commit --no-verify' to bypass this check."
    echo ""
    echo "Limits:"
    echo "  - Max $MAX_LINES_FILE lines per file (excluding blank/comment lines)"
    echo "  - Max $MAX_LINES_FUNCTION lines per function"
    echo "  - Max $MAX_CCN cyclomatic complexity per function"
    echo ""
    exit 1
else
    echo -e "${GREEN}✓ All staged files pass code metrics checks${NC}"
    exit 0
fi
