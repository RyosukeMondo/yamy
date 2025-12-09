#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
SRC_DIR="$REPO_ROOT/src"

echo -e "\033[36mChecking for UTF-8 BOM in source files...\033[0m"

BAD_ENCODING=()

# Find all relevant source files
while IFS= read -r -d '' file; do
    relative_path="${file#$REPO_ROOT/}"

    # Exclude 3rd party/tests if needed
    if [[ "$relative_path" =~ ^src/tests/ ]] || [[ "$relative_path" =~ ^src/ts4mayu/ ]]; then
        continue
    fi

    # Check for UTF-8 BOM (0xEF 0xBB 0xBF)
    if [[ -f "$file" ]]; then
        # Read first 3 bytes
        bom=$(head -c 3 "$file" | od -An -tx1 | tr -d ' ')

        # Skip files smaller than 3 bytes
        if [[ ${#bom} -lt 6 ]]; then
            continue
        fi

        if [[ "$bom" != "efbbbf" ]]; then
            BAD_ENCODING+=("$relative_path")
        fi
    fi
done < <(find "$SRC_DIR" -type f \( -name "*.h" -o -name "*.hpp" -o -name "*.cpp" -o -name "*.c" \) -print0)

if [[ ${#BAD_ENCODING[@]} -gt 0 ]]; then
    echo -e "\033[31mFound ${#BAD_ENCODING[@]} files missing UTF-8 BOM:\033[0m"
    for missing in "${BAD_ENCODING[@]}"; do
        echo -e "\033[31m  - $missing\033[0m"
    done
    echo -e "\n\033[33mMSVC may misinterpret non-ASCII characters in these files.\033[0m"
    echo -e "\033[33mConsider converting them to UTF-8 with BOM.\033[0m"
    exit 1
else
    echo -e "\033[32mAll source files have UTF-8 BOM.\033[0m"
    exit 0
fi
