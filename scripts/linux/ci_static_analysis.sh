#!/bin/bash

# Exit on error
set -e

echo "=== Static Analysis: Windows Dependencies ==="

# 1. Check for windows.h includes in src/core
echo "Checking for <windows.h> includes in src/core..."
# Find files with #include <windows.h> and check if each is guarded by #ifdef _WIN32
UNGUARDED_COUNT=0
UNGUARDED_FILES=""

while IFS=: read -r file line; do
    # Check if the line before #include <windows.h> is #ifdef _WIN32
    line_num=$(grep -n "#include <windows.h>" "$file" | head -1 | cut -d: -f1)
    if [ -n "$line_num" ] && [ "$line_num" -gt 1 ]; then
        prev_line=$(sed -n "$((line_num - 1))p" "$file")
        if echo "$prev_line" | grep -q "#ifdef _WIN32"; then
            # Properly guarded, skip
            continue
        fi
    fi
    UNGUARDED_COUNT=$((UNGUARDED_COUNT + 1))
    UNGUARDED_FILES="$UNGUARDED_FILES\n$file"
done < <(grep -rl "#include <windows.h>" src/core 2>/dev/null | while read f; do echo "$f:1"; done)

if [ "$UNGUARDED_COUNT" -ne 0 ]; then
    echo "Error: Found $UNGUARDED_COUNT unguarded inclusion(s) of <windows.h> in src/core:"
    echo -e "$UNGUARDED_FILES"
    exit 1
else
    echo "Pass: No unguarded <windows.h> includes found in src/core."
    echo "   (All <windows.h> includes are properly guarded with #ifdef _WIN32)"
fi

# 2. Check for Win32 specific types in src/core
echo ""
echo "Checking for Win32 types (HWND, DWORD, MSG, WPARAM, LPARAM) in src/core..."
WIN32_TYPES="HWND|DWORD|MSG|WPARAM|LPARAM"
TYPE_COUNT=$(grep -rE "$WIN32_TYPES" src/core | wc -l)

# Threshold set to current count to prevent regression.
# As Track 3 progresses, this number should decrease.
THRESHOLD=70

echo "Found $TYPE_COUNT usages of Win32 types."

if [ "$TYPE_COUNT" -gt "$THRESHOLD" ]; then
    echo "❌ Error: Win32 type usage ($TYPE_COUNT) exceeds threshold ($THRESHOLD)."
    echo "This indicates new Win32 dependencies have been introduced."
    exit 1
else
    echo "✅ Pass: Win32 type usage within threshold."
fi

echo ""
echo "=== Static Analysis Complete ==="
