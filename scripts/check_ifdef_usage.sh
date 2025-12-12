#!/bin/bash
# Check #ifdef usage in the codebase and categorize by severity

set -e

REPO_ROOT=$(git rev-parse --show-toplevel)
SRC_DIR="$REPO_ROOT/src/core"

echo "=== #ifdef Usage Analysis ==="
echo ""

# Count total #ifdef/_WIN32 occurrences
TOTAL_IFDEF=$(grep -r "#ifdef _WIN32\|#ifndef _WIN32" "$SRC_DIR" --include="*.cpp" --include="*.h" | wc -l)

# Exclude "acceptable pattern" files from count
ACCEPTABLE_FILES=(
    "src/core/platform/platform_time.h"
    "src/core/platform/shell_constants.h"
    "src/core/platform/message_constants.h"
    "src/core/window/target.cpp"
    "src/core/window/layoutmanager.cpp"
    "src/core/window/focus.cpp"
    "src/core/input/vk_constants.h"
    "src/core/settings/config_watcher.h"
)

ACCEPTABLE_COUNT=0
for file in "${ACCEPTABLE_FILES[@]}"; do
    if [ -f "$REPO_ROOT/$file" ]; then
        count=$(grep "#ifdef _WIN32\|#ifndef _WIN32" "$REPO_ROOT/$file" 2>/dev/null | wc -l || echo 0)
        ACCEPTABLE_COUNT=$((ACCEPTABLE_COUNT + count))
    fi
done

SHOULD_REFACTOR=$((TOTAL_IFDEF - ACCEPTABLE_COUNT))

echo "Total #ifdef _WIN32 count: $TOTAL_IFDEF"
echo "  - Acceptable pattern (platform abstractions): $ACCEPTABLE_COUNT"
echo "  - Should refactor (scattered in implementation): $SHOULD_REFACTOR"
echo ""

# Categorize by severity
echo "=== Severity Categories ==="
echo ""

# Category 1: Inline conditionals (WORST - hard to read)
echo "1. INLINE CONDITIONALS (worst - breaks code flow):"
echo "   Files with #ifdef in middle of functions:"
grep -r "#ifdef _WIN32" "$SRC_DIR" --include="*.cpp" -B 3 | \
  grep -E "^\s+#ifdef _WIN32" | \
  sed 's/:.*#ifdef/_WIN32/' | \
  cut -d: -f1 | \
  sort | uniq -c | sort -rn | head -20
echo ""

INLINE_COUNT=$(grep -r "#ifdef _WIN32" "$SRC_DIR" --include="*.cpp" -B 3 | \
  grep -E "^\s+#ifdef _WIN32" | wc -l)
echo "   Total inline #ifdef count: $INLINE_COUNT"
echo ""

# Category 2: Function-level conditionals (ACCEPTABLE - clear boundaries)
echo "2. FUNCTION-LEVEL CONDITIONALS (acceptable - clear boundaries):"
echo "   Examples: entire function body wrapped"
grep -r "#ifdef _WIN32" "$SRC_DIR" --include="*.cpp" -A 1 | \
  grep -E "^[^-]*\.(cpp|h)-.*{$" | head -5
echo ""

# Category 3: File-level conditionals (BEST - clean separation)
echo "3. FILE-LEVEL CONDITIONALS (best - clean separation):"
echo "   Files with platform-specific implementations:"
find "$SRC_DIR" -name "*_windows.*" -o -name "*_linux.*" | sort
echo ""

# Top offenders - files with most inline #ifdefs
echo "=== Top 10 Files with Most Inline #ifdefs ==="
for file in $(grep -r "#ifdef _WIN32" "$SRC_DIR" --include="*.cpp" -l); do
  count=$(grep "#ifdef _WIN32" "$file" | wc -l)
  echo "$count $file"
done | sort -rn | head -10
echo ""

# Detailed breakdown by directory
echo "=== #ifdef Count by Directory ==="
for dir in "$SRC_DIR"/{engine,commands,functions,settings,input}; do
  if [ -d "$dir" ]; then
    count=$(grep -r "#ifdef _WIN32\|#ifndef _WIN32" "$dir" 2>/dev/null | wc -l || echo 0)
    dirname=$(basename "$dir")
    printf "%-20s %d\n" "$dirname:" "$count"
  fi
done
echo ""

# Recommendations
echo "=== Recommendations ==="
echo ""
echo "Priorities for refactoring (highest to lowest):"
echo "1. Remove INLINE conditionals (breaks code flow)"
echo "2. Convert to FUNCTION-LEVEL conditionals (one function = one platform)"
echo "3. Eventually: Split to separate files (file-level separation)"
echo ""
echo "Acceptable #ifdef usage:"
echo "  ✓ Entire function implementation switched by platform"
echo "  ✓ Header include guards"
echo "  ✓ Type definitions at file top"
echo ""
echo "Unacceptable #ifdef usage:"
echo "  ✗ Mid-function conditionals (inline)"
echo "  ✗ Single statement switches scattered everywhere"
echo "  ✗ Nested #ifdef blocks"
echo ""

# Generate metric for CI tracking (save total count, not inline count)
METRIC_FILE="$REPO_ROOT/.github/metrics/ifdef_count.txt"
METRIC_FILE_REFACTOR="$REPO_ROOT/.github/metrics/ifdef_should_refactor.txt"
mkdir -p "$(dirname "$METRIC_FILE")"
echo "$TOTAL_IFDEF" > "$METRIC_FILE"
echo "$SHOULD_REFACTOR" > "$METRIC_FILE_REFACTOR"
echo "Metrics saved:"
echo "  - Total: $METRIC_FILE ($TOTAL_IFDEF)"
echo "  - Should refactor: $METRIC_FILE_REFACTOR ($SHOULD_REFACTOR)"
echo "  - Inline: $INLINE_COUNT"
