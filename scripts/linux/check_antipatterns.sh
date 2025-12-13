#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_ROOT="$REPO_ROOT/src"

echo -e "\033[36mChecking for code anti-patterns in: $SRC_ROOT\033[0m"

ERROR_COUNT=0
WARNING_COUNT=0

# Function to check if path should be excluded
should_exclude() {
    local path="$1"
    local exclude_patterns="$2"

    if [[ -z "$exclude_patterns" ]]; then
        return 1  # Don't exclude
    fi

    IFS='|' read -ra patterns <<< "$exclude_patterns"
    for pattern in "${patterns[@]}"; do
        if [[ "$path" =~ $pattern ]]; then
            return 0  # Exclude
        fi
    done
    return 1  # Don't exclude
}

# Find all relevant files (excluding googletest)
mapfile -t FILES < <(find "$SRC_ROOT" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.c" \) | grep -v googletest)

echo "DEBUG: Found ${#FILES[@]} files to check" >&2

# Check each file
for file in "${FILES[@]}"; do
    relative_path="${file#$REPO_ROOT/}"

    # Check 1: Legacy NULL
    if ! should_exclude "$relative_path" "src/tests/|src/ts4mayu/"; then
        while IFS=: read -r line_num _; do
            echo -e "\033[31m[Error] Legacy NULL: $relative_path:$line_num - Use 'nullptr' instead of 'NULL'.\033[0m"
            ((ERROR_COUNT++))
        done < <(grep -n '\bNULL\b' "$file" 2>/dev/null || true)
    fi

    # Check 2: Namespace pollution (headers only)
    if [[ "$file" =~ \.h$ ]] && ! should_exclude "$relative_path" "src/tests/"; then
        while IFS=: read -r line_num _; do
            echo -e "\033[31m[Error] Namespace Pollution: $relative_path:$line_num - Do not use 'using namespace std;' in header files.\033[0m"
            ((ERROR_COUNT++))
        done < <(grep -n '^using\s\+namespace\s\+std;' "$file" 2>/dev/null || true)
    fi

    # Check 3: Unsafe string functions
    if ! should_exclude "$relative_path" "src/tests/|src/ts4mayu/"; then
        while IFS=: read -r line_num _; do
            echo -e "\033[33m[Warning] Unsafe String Function: $relative_path:$line_num - Use safe alternatives like 'snprintf', 'strncpy', or std::string.\033[0m"
            ((WARNING_COUNT++))
        done < <(grep -nE '\b(sprintf|strcpy|strcat)\b' "$file" 2>/dev/null || true)
    fi

    # Check 4: Raw memory allocation
    if ! should_exclude "$relative_path" "src/tests/|src/ts4mayu/|src/platform/"; then
        while IFS=: read -r line_num _; do
            echo -e "\033[33m[Warning] Raw Memory Allocation: $relative_path:$line_num - Use 'new'/'delete' or smart pointers.\033[0m"
            ((WARNING_COUNT++))
        done < <(grep -nE '\b(malloc|free)\b' "$file" 2>/dev/null || true)
    fi

    # Check 5: Source file inclusion
    if ! should_exclude "$relative_path" "src/tests/"; then
        while IFS=: read -r line_num _; do
            echo -e "\033[31m[Error] Source File Inclusion: $relative_path:$line_num - Do not include .cpp files.\033[0m"
            ((ERROR_COUNT++))
        done < <(grep -nE '^\s*#include\s+["<].+\.cpp[">]' "$file" 2>/dev/null || true)
    fi

    # Check 6: Conditional compilation
    if ! should_exclude "$relative_path" "src/tests/|src/ts4mayu/|src/platform/|src/core/engine/engine.h"; then
        while IFS=: read -r line_num _; do
            echo -e "\033[33m[Warning] Conditional Compilation in Core: $relative_path:$line_num - Minimize #ifdef in core logic. Use interfaces/polymorphism.\033[0m"
            ((WARNING_COUNT++))
        done < <(grep -nE '^\s*#ifn?def\s+' "$file" 2>/dev/null || true)
    fi

    # Check 7: Tab characters (skip - too many to report)
    # if ! should_exclude "$relative_path" "src/tests/|src/ts4mayu/"; then
    #     tab_count=$(grep -c $'\t' "$file" 2>/dev/null || echo "0")
    #     if [[ $tab_count -gt 0 ]]; then
    #         echo -e "\033[33m[Warning] Tab Character: $relative_path - File contains $tab_count lines with tabs.\033[0m"
    #         ((WARNING_COUNT+=tab_count))
    #     fi
    # fi
done

echo "DEBUG: Loop completed" >&2
echo ""
echo "Anti-pattern check complete."
echo "DEBUG: ERROR_COUNT=$ERROR_COUNT, WARNING_COUNT=$WARNING_COUNT"

if [[ $ERROR_COUNT -gt 0 ]]; then
    echo -e "\033[31mErrors: $ERROR_COUNT\033[0m"
else
    echo -e "\033[32mErrors: $ERROR_COUNT\033[0m"
fi

if [[ $WARNING_COUNT -gt 0 ]]; then
    echo -e "\033[33mWarnings: $WARNING_COUNT\033[0m"
else
    echo -e "\033[32mWarnings: $WARNING_COUNT\033[0m"
fi

if [[ $ERROR_COUNT -gt 0 ]]; then
    echo -e "\033[31mFound $ERROR_COUNT anti-pattern errors. Build failed.\033[0m"
    echo "DEBUG: Exiting with 1"
    exit 1
fi

echo "DEBUG: Exiting with 0"
exit 0
