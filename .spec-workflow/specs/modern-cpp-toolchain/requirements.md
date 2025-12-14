# Requirements Document: Modern C++ Toolchain

## Overview

This specification modernizes YAMY's C++ development stack to achieve <5-second iteration cycles while maintaining <1ms runtime latency. The modernization focuses on build infrastructure (Mold/LLD linkers, Ninja, ccache), dependency management (Conan 2.0), observability (Quill logging), correctness (Microsoft GSL contracts), and testing (RapidCheck property-based testing), while ensuring AI agent compatibility.

**Key Exclusion**: Hot reload functionality (cr/Live++) is EXCLUDED due to architectural invasiveness and limited benefit for YAMY's use case.

---

## Requirements

### REQ-1: Fast Incremental Linking

**As a** developer making small code changes
**I want** incremental rebuilds to complete in under 5 seconds
**So that** I maintain high development velocity without context switching

**Priority**: CRITICAL
**Category**: Build Performance

**Acceptance Criteria**:
1. Linux builds use Mold linker (10x faster than GNU ld)
2. Windows builds use LLD linker with clang-cl (3-5x faster than MSVC link.exe)
3. CMake automatically detects and configures the appropriate linker
4. Incremental build (single file change) completes in <5s on reference hardware
5. Null builds (no changes) complete in <1s

**Dependencies**: CMake 3.28+, Ninja build system
**Testing**: Benchmark suite measuring build times for various change scenarios

---

### REQ-2: Build Result Caching

**As a** developer switching between Git branches
**I want** previously compiled files to be reused from cache
**So that** I don't waste time recompiling unchanged code

**Priority**: HIGH
**Category**: Build Performance

**Acceptance Criteria**:
1. ccache is integrated into the CMake build system
2. ccache is automatically detected and configured when available
3. Cache hit rate >80% when switching between related branches
4. Cache storage is configurable (default 5GB)
5. Cache statistics are accessible via `ccache -s`

**Dependencies**: ccache, CMake compiler launcher support
**Testing**: Measure cache hit rates across branch switches, verify rebuild times

---

### REQ-3: Reproducible Dependency Management

**As a** developer or CI system
**I want** all third-party dependencies to be fetched and built deterministically
**So that** "works on my machine" problems are eliminated

**Priority**: CRITICAL
**Category**: Build Infrastructure

**Acceptance Criteria**:
1. Conan 2.0 manages all third-party dependencies (Quill, GSL, RapidCheck, Catch2, fmt)
2. `conanfile.txt` specifies exact dependency versions
3. Conan generates `conan_toolchain.cmake` for CMake integration
4. Binary cache support (local or remote) eliminates rebuild of dependencies
5. Clean builds on CI complete in <5 minutes (with binary cache)

**Dependencies**: Conan 2.0, CMake 3.28+
**Testing**: Clean build on fresh machine, verify all dependencies fetch correctly

---

### REQ-4: Zero-Latency Structured Logging

**As a** developer debugging input latency issues
**I want** logging that introduces <1μs overhead on the critical path
**So that** I can diagnose issues without impacting runtime performance

**Priority**: CRITICAL
**Category**: Observability

**Acceptance Criteria**:
1. Quill logging library integrated into core engine
2. Logging uses SPSC ring buffers (no mutex contention on hot path)
3. RDTSC timestamping (no syscall overhead)
4. Structured JSON output for AI-compatible analysis
5. Measured latency <1μs for `LOG_INFO` on critical path (99th percentile)

**Dependencies**: Quill 4.1.0+, Conan integration
**Testing**: Microbenchmarks for logging latency, integration tests for JSON output

---

### REQ-5: Contract-Based Programming

**As a** developer writing safety-critical input handling code
**I want** preconditions and postconditions to be checked in debug builds
**So that** state machine bugs (stuck keys) are caught immediately

**Priority**: HIGH
**Category**: Correctness

**Acceptance Criteria**:
1. Microsoft GSL integrated into the project
2. All public APIs in core engine use `Expects()` preconditions
3. Critical functions use `Ensures()` postconditions
4. Array parameters use `gsl::span` instead of pointer+size
5. Debug builds trap on contract violations (debugger breakpoint)
6. Release builds optimize contracts out (zero runtime cost)

**Dependencies**: Microsoft GSL (ms-gsl 4.0.0+), Conan integration
**Testing**: Unit tests verifying contract violations crash in debug builds

---

### REQ-6: Property-Based State Machine Testing

**As a** developer testing keyboard remapping state machines
**I want** automatic generation of input sequences that trigger bugs
**So that** I find edge cases that unit tests miss

**Priority**: HIGH
**Category**: Testing

**Acceptance Criteria**:
1. RapidCheck integrated into test suite
2. Properties defined for critical invariants (e.g., "all key-down events have matching key-up")
3. RapidCheck shrinks failing test cases to minimal reproduction
4. Property tests run on all state machine components (modifier tracking, layer switching)
5. Coverage: At least 3 properties per state machine component

**Dependencies**: RapidCheck, Catch2 integration
**Testing**: Write property test that intentionally fails, verify shrinking works

---

### REQ-7: CMakePresets.json for AI Compatibility

**As an** AI agent or developer
**I want** a single command to configure, build, and test the project
**So that** build configuration is deterministic and documented

**Priority**: HIGH
**Category**: Build Infrastructure

**Acceptance Criteria**:
1. `CMakePresets.json` defines presets for all build configurations (debug/release × Linux/Windows)
2. Presets specify: generator (Ninja), toolchain file (Conan), compiler, linker
3. AI agents can build with `cmake --preset linux-debug && cmake --build --preset linux-debug`
4. No manual CMake flags required (`-DCMAKE_BUILD_TYPE`, etc.)
5. Presets are version-controlled

**Dependencies**: CMake 3.28+, Ninja, Conan toolchain
**Testing**: Clean clone → configure with preset → build → test (automated in CI)

---

### REQ-8: AI-Compatible Project Structure

**As an** AI agent (Claude, GPT-4o) assisting with development
**I want** a codebase map and coding rules file
**So that** I can navigate the project and follow conventions without loading all files

**Priority**: MEDIUM
**Category**: Developer Experience

**Acceptance Criteria**:
1. `docs/map.md` exists with concise descriptions of all major files
2. `.clinerules` and `.cursorrules` files define coding standards and architecture constraints
3. Headers use Doxygen comments (AI-readable intent), implementations are sparse
4. AI agent can locate relevant files in <3 queries (measured empirically)
5. Coding rules are automatically enforced by pre-commit hooks

**Dependencies**: None (documentation only)
**Testing**: Ask AI agent to locate "input event processing code" without context, measure queries

---

### REQ-9: Code Metrics Enforcement

**As a** project maintainer
**I want** code complexity and file size to be bounded
**So that** the codebase remains maintainable as it grows

**Priority**: MEDIUM
**Category**: Code Quality

**Acceptance Criteria**:
1. Max 500 lines per file (excluding comments/blank lines)
2. Max 50 lines per function
3. Max cyclomatic complexity 15 per function (10 for critical paths)
4. Metrics enforced via pre-commit hooks (fails commit on violation)
5. CI pipeline runs `make check-metrics` to verify

**Dependencies**: lizard or cppcheck, CMake custom target
**Testing**: Intentionally violate each metric, verify hook/CI rejection

---

### REQ-10: Unified Toolchain Configuration

**As a** developer working on both Linux and Windows
**I want** consistent compiler, linker, and build tool behavior across platforms
**So that** I don't encounter platform-specific build failures

**Priority**: HIGH
**Category**: Build Infrastructure

**Acceptance Criteria**:
1. Linux uses Clang 16+ with Mold linker
2. Windows uses Clang-cl 16+ with LLD linker
3. Both platforms use Ninja build generator
4. Both platforms use CMake 3.28+
5. Both platforms use C++20 standard (`-std=c++20`)
6. Build warnings are errors (`-Werror` / `/WX`)

**Dependencies**: Clang/Clang-cl, Mold, LLD, Ninja, CMake
**Testing**: Same source builds and passes tests on both platforms

---

## Non-Requirements

### Explicitly Excluded

1. **Hot Reload (cr / Live++)**
   - **Rationale**: Requires invasive architectural split (runner/logic separation), limited benefit for YAMY's single-threaded model. Research indicates 5-15s build times are acceptable for keyboard remapper development.
   - **Trade-off**: Developers must restart application after code changes (acceptable with <5s rebuild).

2. **Backward Compatibility with Old Toolchains**
   - **Rationale**: CMake 3.28+, C++20, Clang 16+ required. No support for GCC <11, CMake <3.20, or C++17.
   - **Trade-off**: Developers must update tools (one-time setup cost).

3. **Windows-Specific Build Optimizations**
   - **Rationale**: Focus on Clang-cl + LLD for cross-platform consistency. No MSVC-specific optimizations.
   - **Trade-off**: Slightly slower Windows builds than fully optimized MSVC (acceptable given LLD speedup).

---

## Success Metrics

| Metric | Baseline | Target | Measurement |
|--------|----------|--------|-------------|
| **Incremental build time** | 30-60s | <5s | Benchmark: Single-file change in engine.cpp |
| **Null build time** | 10s | <1s | `cmake --build --preset linux-debug` (no changes) |
| **Clean build time (with cache)** | 10 min | <2 min | CI pipeline on fresh runner |
| **Logging overhead** | ~10μs (spdlog) | <1μs | Microbenchmark on critical path |
| **Code coverage (with property tests)** | 60% | 80% | gcov/lcov report |
| **AI agent file location** | N/A | <3 queries | Empirical test with Claude/GPT-4o |

---

## Risk Analysis

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| **Mold linker instability** | Build failures | Low | Fallback to LLD on Linux, extensive testing |
| **Conan dependency unavailability** | Build blocked | Low | Pin specific versions, maintain local cache |
| **Quill learning curve** | Slow adoption | Medium | Provide wrapper API, comprehensive examples |
| **RapidCheck false positives** | Test noise | Medium | Carefully design properties, use RC_CLASSIFY |
| **Toolchain version skew** | Platform divergence | Medium | Enforce versions in CMakePresets.json, CI checks |
| **Code metrics too strict** | Developer friction | Medium | Iteratively tune limits based on feedback |

---

## Validation Plan

### Phase 1: Toolchain Setup (Week 1)
- Install Mold, Clang, Ninja, Conan on development machines
- Create `CMakePresets.json` with initial configurations
- Verify clean build on Linux and Windows

### Phase 2: Dependency Integration (Week 2)
- Add Quill, GSL, RapidCheck to `conanfile.txt`
- Verify Conan generates toolchain correctly
- Test binary cache functionality

### Phase 3: Code Integration (Weeks 3-4)
- Replace existing logging with Quill
- Add GSL contracts to core engine APIs
- Write initial property-based tests

### Phase 4: Metrics & Validation (Week 5)
- Measure build times, logging latency
- Run full test suite with coverage
- Document AI agent navigation tests

---

**Document Version**: 1.0
**Created**: 2025-12-15
**Author**: AI Specification Generator
**Reviewed By**: (Pending approval)
