# Code Quality Guidelines

This document describes the code quality tools and standards used in the YAMY project.

## Overview

YAMY uses the following tools for code quality enforcement:
- **clang-format**: Automatic code formatting
- **clang-tidy**: Static analysis and linting

Both tools run automatically in CI on every push and pull request.

## Code Formatting (clang-format)

### Configuration

The project uses a `.clang-format` file at the repository root with the following key settings:
- Based on Google style
- 4-space indentation
- 100 column limit
- Pointer/reference alignment: left (`int* ptr` not `int *ptr`)
- C++17 standard

### Running Locally

#### Check formatting (without modifying files)
```bash
./scripts/format-code.sh --check
```

#### Format all files in src/
```bash
./scripts/format-code.sh
```

#### Format specific directory or file
```bash
./scripts/format-code.sh src/platform/linux/
./scripts/format-code.sh src/platform/linux/input_driver_linux.cpp
```

#### Manual clang-format commands
```bash
# Check single file
clang-format --dry-run --Werror file.cpp

# Format single file in-place
clang-format -i file.cpp
```

### Installation

```bash
# Ubuntu/Debian
sudo apt-get install clang-format

# Fedora
sudo dnf install clang-tools-extra

# macOS
brew install clang-format
```

## Static Analysis (clang-tidy)

### Configuration

The `.clang-tidy` file at the repository root enables a curated set of checks:
- **bugprone-***: Common bug patterns
- **clang-analyzer-***: Static analyzer checks
- **cppcoreguidelines-***: C++ Core Guidelines (subset)
- **modernize-***: Modern C++ recommendations
- **performance-***: Performance improvements
- **readability-***: Code readability

Some noisy checks are disabled to reduce false positives.

### Running Locally

#### Analyze platform/linux code (default)
```bash
./scripts/run-clang-tidy.sh
```

#### Analyze specific directory or file
```bash
./scripts/run-clang-tidy.sh src/platform/linux/
./scripts/run-clang-tidy.sh src/core/input/keymap.cpp
```

#### Automatically apply fixes
```bash
./scripts/run-clang-tidy.sh --fix src/platform/linux/file.cpp
```

#### Manual clang-tidy command
```bash
clang-tidy file.cpp -- -std=c++17 -I src
```

### Installation

```bash
# Ubuntu/Debian
sudo apt-get install clang-tidy

# Fedora
sudo dnf install clang-tools-extra

# macOS
brew install llvm  # clang-tidy is part of LLVM
```

## CI Integration

The CI workflow runs code quality checks on every push and pull request:

1. **Formatting Check**: Verifies all source files match the `.clang-format` style
   - Reports files that need formatting
   - Currently warns but doesn't fail the build (gradual adoption)

2. **clang-tidy Analysis**: Runs static analysis on platform code
   - Reports potential issues
   - Currently informational (doesn't fail build)

### Making CI Enforcement Strict

To enforce formatting as a required check, modify `.github/workflows/ci.yml`:
```yaml
# Change from warning to failure
echo "::warning::Code formatting issues found"
# To:
echo "::error::Code formatting issues found"
exit 1
```

## IDE Integration

### VS Code

Install the [C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) and add to `.vscode/settings.json`:

```json
{
  "C_Cpp.clang_format_style": "file",
  "editor.formatOnSave": true,
  "[cpp]": {
    "editor.defaultFormatter": "ms-vscode.cpptools"
  }
}
```

### CLion

CLion automatically detects `.clang-format` and `.clang-tidy` files. Enable:
- Settings > Editor > Code Style > C/C++ > Enable ClangFormat
- Settings > Editor > Inspections > C/C++ > Clang-Tidy

### Vim/Neovim

Add to your configuration:
```vim
" Auto-format on save
autocmd BufWritePre *.cpp,*.h :silent! !clang-format -i %

" Or with a mapping
nnoremap <leader>cf :!clang-format -i %<CR>
```

## Pre-commit Hook (Optional)

To run formatting checks before every commit, add a pre-commit hook:

```bash
#!/bin/bash
# .git/hooks/pre-commit

# Check formatting of staged C++ files
STAGED=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|h)$')

if [ -n "$STAGED" ]; then
  for FILE in $STAGED; do
    if ! clang-format --dry-run --Werror "$FILE" 2>/dev/null; then
      echo "Error: $FILE needs formatting. Run: clang-format -i $FILE"
      exit 1
    fi
  done
fi
```

Make it executable:
```bash
chmod +x .git/hooks/pre-commit
```

## Style Guidelines Summary

1. **Indentation**: 4 spaces, no tabs
2. **Line length**: Maximum 100 characters
3. **Braces**: Same line as control statements (`if (x) {`)
4. **Pointers**: Align left (`int* ptr`)
5. **Includes**: Sorted case-insensitively, grouped by category
6. **Namespaces**: No indentation inside namespaces
7. **Naming**:
   - Classes/Structs: `CamelCase`
   - Functions/Methods: `camelBack`
   - Variables: `camelBack`
   - Constants/Macros: `UPPER_CASE`
   - Private members: `member_` (trailing underscore)
