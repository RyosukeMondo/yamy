#!/bin/bash

# Exit on error
set -e

echo "=== Static Analysis: Windows Dependencies ==="

# 1. Check for windows.h includes in src/core
echo "Checking for <windows.h> includes in src/core..."
WINDOWS_H_COUNT=$(grep -r "#include <windows.h>" src/core | wc -l)

if [ "$WINDOWS_H_COUNT" -ne 0 ]; then
    echo "❌ Error: Found $WINDOWS_H_COUNT inclusion(s) of <windows.h> in src/core:"
    grep -r "#include <windows.h>" src/core
    exit 1
else
    echo "✅ Pass: No <windows.h> includes found in src/core."
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
