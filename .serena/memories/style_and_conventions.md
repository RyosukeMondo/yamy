# Style and Conventions
- Formatting: `.clang-format` (Google-based) with 4-space indent, 100 column limit, attach braces, no tabs; pointers/references aligned left; namespaces not indented; empty lines kept minimal.
- clang-tidy: `.clang-tidy` enables bugprone/modernize/performance/readability with tuned suppressions. Naming: Classes/Structs CamelCase; functions camelBack; variables camelBack; members camelBack with trailing `_`; constants/macros UPPER_CASE; namespaces lower_case. Function size thresholds enforced (100 lines default threshold).
- C++ standard: C++17 per clang-format config. Avoid Windows type leakage in core; prefer platform abstractions.
- Comments: concise; favor self-explanatory code. Add explanatory comment only for non-obvious logic.
- Includes: sorted case-insensitive with project headers grouped; prefer project headers before std headers after categories defined.
- Build targets: Qt5 GUI app; keep Linux stubs clean of Windows deps.
- Config files: `.mayu` keymap syntax used in keymaps/ docs.