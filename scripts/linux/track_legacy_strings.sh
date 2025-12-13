#!/bin/bash
# scripts/track_legacy_strings.sh
# Track legacy string usage and Win32 type leakage in Yamy codebase

set -e

echo "============================================"
echo "  Legacy String Usage Tracking"
echo "============================================"
echo ""

# Legacy string usage counts
echo "=== TCHAR/tstring Legacy Usage ==="
TSTRING_COUNT=$(grep -r "tstring" src/core --include="*.cpp" --include="*.h" 2>/dev/null | wc -l || echo "0")
T_MACRO_COUNT=$(grep -r "_T(" src/core --include="*.cpp" --include="*.h" 2>/dev/null | wc -l || echo "0")
TCHAR_COUNT=$(grep -r "_TCHAR" src/core --include="*.cpp" --include="*.h" 2>/dev/null | wc -l || echo "0")
TSTRINGI_COUNT=$(grep -r "tstringi" src/core --include="*.cpp" --include="*.h" 2>/dev/null | wc -l || echo "0")

echo "tstring usages: $TSTRING_COUNT"
echo "_T() macro usages: $T_MACRO_COUNT"
echo "_TCHAR usages: $TCHAR_COUNT"
echo "tstringi usages: $TSTRINGI_COUNT"
echo ""

TOTAL_LEGACY=$((TSTRING_COUNT + T_MACRO_COUNT + TCHAR_COUNT + TSTRINGI_COUNT))
echo "TOTAL legacy string usages: $TOTAL_LEGACY"
echo ""

# Win32 type leakage
echo "=== Win32 Type Leakage ==="
WIN32_USAGE_COUNT=$(grep -r "HWND\|DWORD\|MSG\|WPARAM\|LPARAM\|RECT\*" src/core --include="*.cpp" --include="*.h" 2>/dev/null | wc -l || echo "0")
WIN32_FILES_COUNT=$(grep -rl "HWND\|DWORD\|MSG\|WPARAM\|LPARAM\|RECT\*" src/core --include="*.cpp" --include="*.h" 2>/dev/null | wc -l || echo "0")

echo "Win32 type usages: $WIN32_USAGE_COUNT"
echo "Files with Win32 types: $WIN32_FILES_COUNT"
echo ""

if [ $WIN32_FILES_COUNT -gt 0 ]; then
    echo "Files with Win32 type leakage:"
    grep -rl "HWND\|DWORD\|MSG\|WPARAM\|LPARAM\|RECT\*" src/core --include="*.cpp" --include="*.h" 2>/dev/null | sed 's/^/  - /' || true
    echo ""
fi

# windows.h includes check
echo "=== windows.h Includes in Core ==="
WINDOWS_H_COUNT=$(grep -r "#include.*windows\.h" src/core --include="*.cpp" --include="*.h" 2>/dev/null | wc -l || echo "0")
echo "windows.h includes in src/core: $WINDOWS_H_COUNT"

if [ $WINDOWS_H_COUNT -gt 0 ]; then
    echo "⚠️  WARNING: windows.h found in core! Files:"
    grep -rl "#include.*windows\.h" src/core --include="*.cpp" --include="*.h" 2>/dev/null | sed 's/^/  - /' || true
else
    echo "✅ CLEAN: No windows.h includes in src/core"
fi
echo ""

# Progress summary
echo "============================================"
echo "  Migration Progress Summary"
echo "============================================"
if [ $TOTAL_LEGACY -lt 400 ]; then
    echo "✅ Phase 3 (String Unification): ON TRACK (<400 usages)"
else
    echo "⚠️  Phase 3 (String Unification): $TOTAL_LEGACY usages remaining"
fi

if [ $WIN32_FILES_COUNT -lt 3 ]; then
    echo "✅ Phase 4 (PAL Completion): ON TRACK (<3 files with leakage)"
else
    echo "⚠️  Phase 4 (PAL Completion): $WIN32_FILES_COUNT files with Win32 leakage"
fi

if [ $WINDOWS_H_COUNT -eq 0 ]; then
    echo "✅ Architecture: CLEAN (zero windows.h in core)"
else
    echo "❌ Architecture: BROKEN (windows.h found in core)"
fi
echo ""
