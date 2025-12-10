#!/bin/bash
# Track legacy string usage and Win32 type leakage
# Part of Yamy modernization effort

set -e

CORE_DIR="src/core"

echo "======================================================"
echo "  Yamy Modernization - Progress Tracking"
echo "======================================================"
echo ""

echo "=== Legacy String Usage ==="
echo ""

# Count tstring usages
TSTRING_COUNT=$(grep -r "tstring" $CORE_DIR --include="*.cpp" --include="*.h" 2>/dev/null | wc -l)
echo "  tstring usages:        $TSTRING_COUNT"

# Count _T() macro usages
T_MACRO_COUNT=$(grep -r "_T(" $CORE_DIR --include="*.cpp" --include="*.h" 2>/dev/null | wc -l)
echo "  _T() macro usages:     $T_MACRO_COUNT"

# Count _TCHAR usages
TCHAR_COUNT=$(grep -r "_TCHAR" $CORE_DIR --include="*.cpp" --include="*.h" 2>/dev/null | wc -l)
echo "  _TCHAR usages:         $TCHAR_COUNT"

# Count tregex usages
TREGEX_COUNT=$(grep -r "tregex" $CORE_DIR --include="*.cpp" --include="*.h" 2>/dev/null | wc -l)
echo "  tregex usages:         $TREGEX_COUNT"

TOTAL_LEGACY_STRINGS=$((TSTRING_COUNT + T_MACRO_COUNT + TCHAR_COUNT + TREGEX_COUNT))
echo "  ----------------------------------------"
echo "  TOTAL LEGACY STRINGS:  $TOTAL_LEGACY_STRINGS"

echo ""
echo "=== Win32 Type Leakage ==="
echo ""

# Count Win32 type usages
WIN32_USAGE_COUNT=$(grep -rE "HWND|DWORD|WPARAM|LPARAM|UINT|LONG[^_]" $CORE_DIR --include="*.cpp" --include="*.h" 2>/dev/null | grep -v "// Windows" | wc -l)
echo "  Win32 type usages:     $WIN32_USAGE_COUNT"

# Count files with Win32 types
WIN32_FILE_COUNT=$(grep -rlE "HWND|DWORD|WPARAM|LPARAM|UINT|LONG[^_]" $CORE_DIR --include="*.cpp" --include="*.h" 2>/dev/null | wc -l)
echo "  Files with Win32 types: $WIN32_FILE_COUNT"

# Count windows.h includes in core
WINDOWS_H_COUNT=$(grep -r "#include <windows.h>" $CORE_DIR --include="*.cpp" --include="*.h" 2>/dev/null | wc -l)
echo "  #include <windows.h>:  $WINDOWS_H_COUNT"

echo ""
echo "=== Files with Win32 Type Leakage ==="
grep -rlE "HWND|DWORD|WPARAM|LPARAM|UINT|LONG[^_]" $CORE_DIR --include="*.cpp" --include="*.h" 2>/dev/null | sed 's/^/  - /' || echo "  (none)"

echo ""
echo "=== Codebase Statistics ==="
echo ""

# Count total core files
TOTAL_FILES=$(find $CORE_DIR -name "*.cpp" -o -name "*.h" | wc -l)
echo "  Total core files:      $TOTAL_FILES"

# Count files with legacy strings
FILES_WITH_LEGACY=$(grep -rlE "tstring|_T\(|_TCHAR|tregex" $CORE_DIR --include="*.cpp" --include="*.h" 2>/dev/null | wc -l)
echo "  Files with legacy:     $FILES_WITH_LEGACY"

# Calculate percentage
if [ $TOTAL_FILES -gt 0 ]; then
    PERCENTAGE=$((FILES_WITH_LEGACY * 100 / TOTAL_FILES))
    echo "  Legacy percentage:     $PERCENTAGE%"
fi

echo ""
echo "=== Progress Metrics ==="
echo ""

# Phase 3 targets
PHASE3_TARGET=0
PHASE3_PROGRESS=$(awk "BEGIN {printf \"%.1f\", 100 - ($TOTAL_LEGACY_STRINGS / 974 * 100)}")
echo "  Phase 3 (String Unification):"
echo "    Current:  $TOTAL_LEGACY_STRINGS usages (was 974)"
echo "    Progress: ${PHASE3_PROGRESS}% complete"
echo ""

# Phase 4 targets
PHASE4_TARGET=0
PHASE4_PROGRESS=$(awk "BEGIN {printf \"%.1f\", 100 - ($WIN32_FILE_COUNT / 15 * 100)}")
echo "  Phase 4 (PAL Completion):"
echo "    Files with leakage: $WIN32_FILE_COUNT (was 15)"
echo "    windows.h includes: $WINDOWS_H_COUNT (target: 0)"
echo "    Progress: ${PHASE4_PROGRESS}% complete"

echo ""
echo "======================================================"

# Exit with status based on targets
if [ $WINDOWS_H_COUNT -gt 0 ]; then
    echo "  WARNING: windows.h still included in core!"
    exit 1
fi

if [ $TOTAL_LEGACY_STRINGS -eq 0 ] && [ $WIN32_FILE_COUNT -eq 0 ]; then
    echo "  SUCCESS: All modernization targets achieved!"
    exit 0
fi

exit 0
