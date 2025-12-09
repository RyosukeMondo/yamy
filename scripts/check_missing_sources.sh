#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_FILE="$REPO_ROOT/CMakeLists.txt"
SRC_DIR="$REPO_ROOT/src"

echo -e "\033[36mChecking for missing sources in CMakeLists.txt...\033[0m"

# Read CMakeLists.txt content
CMAKE_CONTENT=$(cat "$CMAKE_FILE")

MISSING_FILES=()

# Find all .cpp files
while IFS= read -r -d '' file; do
    # Get relative path from repo root
    relative_path="${file#$REPO_ROOT/}"

    # Check if this path exists in CMakeLists.txt
    if ! echo "$CMAKE_CONTENT" | grep -qF "$relative_path"; then
        # Exclude known non-build files
        if [[ "$relative_path" =~ ^src/tests/ ]] || [[ "$relative_path" =~ ^src/ts4mayu/ ]]; then
            continue
        fi
        MISSING_FILES+=("$relative_path")
    fi
done < <(find "$SRC_DIR" -type f -name "*.cpp" -print0)

if [[ ${#MISSING_FILES[@]} -gt 0 ]]; then
    echo -e "\033[31mFound ${#MISSING_FILES[@]} .cpp files missing from CMakeLists.txt:\033[0m"
    for missing in "${MISSING_FILES[@]}"; do
        echo -e "\033[31m  - $missing\033[0m"
    done
    echo -e "\n\033[33mPlease add these files to CMakeLists.txt to avoid linker errors.\033[0m"
    exit 1
else
    echo -e "\033[32mAll .cpp files appear to be included in CMakeLists.txt.\033[0m"
    exit 0
fi
