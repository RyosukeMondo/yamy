# Tasks Document

## Phase 1: Build System Modernization

- [x] 1.1. Install and configure Mold linker (Linux)
  - File: `CMakeLists.txt`
  - Install Mold linker on development machines and CI
  - Add CMake detection for Mold and automatic configuration
  - Add fallback to LLD if Mold unavailable
  - Purpose: Enable 10x faster linking on Linux
  - _Leverage: CMake find_program, add_link_options
  - _Requirements: 1, 10_
  - _Prompt: **Role:** Build Engineer with expertise in modern C++ linkers and CMake | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Install Mold linker and configure CMake to automatically detect and use it on Linux following requirements 1, 10. Add fallback to LLD if Mold is not available. Verify linking performance improvement. | **Restrictions:** Must work on both development machines and CI, must provide clear error message if Mold installation fails, fallback must be automatic | **_Leverage:** Use find_program to detect Mold, add_link_options("-fuse-ld=mold") for configuration, test on Linux system | **_Requirements:** 1, 10 | **Success:** Mold is detected and used automatically on Linux, linking time for YAMY reduced by >5x compared to GNU ld, fallback to LLD works correctly | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 1.2. Install and configure LLD linker (Windows)
  - File: `CMakeLists.txt`
  - Install LLD (part of LLVM toolchain) on Windows
  - Configure CMake to use lld-link with clang-cl
  - Verify compatibility with existing build
  - Purpose: Enable 3-5x faster linking on Windows
  - _Leverage: CMake compiler detection, clang-cl integration
  - _Requirements: 1, 10_
  - _Prompt: **Role:** Build Engineer with expertise in Windows C++ toolchains and LLVM | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Configure CMake to use LLD linker with clang-cl on Windows following requirements 1, 10. Ensure compatibility with existing YAMY build, verify performance improvement over MSVC linker. | **Restrictions:** Must detect clang-cl compiler, must not break MSVC builds if clang-cl unavailable, provide clear installation instructions | **_Leverage:** Use CMAKE_CXX_COMPILER_ID check, add_link_options with /clang: prefix, LLVM toolchain documentation | **_Requirements:** 1, 10 | **Success:** LLD used automatically with clang-cl on Windows, linking time reduced by >3x vs MSVC link.exe, existing build still works | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 1.3. Configure Ninja build generator
  - File: `CMakePresets.json`
  - Create CMakePresets.json with Ninja as default generator
  - Add presets for debug and release builds on Linux/Windows
  - Verify Ninja is installed on development machines and CI
  - Purpose: Enable fast incremental and null builds
  - _Leverage: CMake 3.28+ presets feature
  - _Requirements: 7, 10_
  - _Prompt: **Role:** Build Engineer with CMake presets expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create CMakePresets.json configuring Ninja as build generator for all platforms following requirements 7, 10. Add presets for debug and release configurations on Linux and Windows. | **Restrictions:** Must specify generator explicitly, presets must be self-contained, must work with Conan toolchain integration | **_Leverage:** CMake presets version 3, generator field, Ninja documentation | **_Requirements:** 7, 10 | **Success:** Ninja used for all builds, cmake --preset linux-debug works without additional flags, null build completes in <1s | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 1.4. Integrate ccache for compilation caching
  - File: `CMakeLists.txt`
  - Add CMake detection for ccache
  - Configure ccache as compiler launcher
  - Set recommended ccache configuration (5GB cache)
  - Purpose: Enable near-instant rebuilds for unchanged files
  - _Leverage: CMAKE_CXX_COMPILER_LAUNCHER
  - _Requirements: 2, 10_
  - _Prompt: **Role:** Build Engineer with ccache optimization expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Integrate ccache into CMake build system following requirements 2, 10. Detect ccache automatically, configure as compiler launcher, set recommended configuration. Verify cache hit rates on branch switches. | **Restrictions:** Must work on Linux and Windows (if ccache available), must be optional (build works without ccache), provide statistics command | **_Leverage:** CMAKE_CXX_COMPILER_LAUNCHER, find_program, ccache -M for cache size config | **_Requirements:** 2, 10 | **Success:** ccache automatically used when available, cache hit rate >80% on branch switches, incremental rebuild time reduced significantly | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 1.5. Benchmark build performance
  - File: `scripts/benchmark_build.sh`
  - Create benchmark script measuring build times
  - Test scenarios: clean build, incremental (1 file), null build
  - Compare baseline (GCC+ld) vs modern (Clang+Mold)
  - Purpose: Validate <5s incremental build target
  - _Leverage: time command, CMake build targets
  - _Requirements: 1, 2_
  - _Prompt: **Role:** Performance Engineer with build system benchmarking expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create build performance benchmark script following requirements 1, 2. Measure clean, incremental, and null build times. Compare baseline vs modern toolchain. Document results. | **Restrictions:** Must test on same hardware, must include warmup runs, must measure wall-clock time, must record tool versions | **_Leverage:** Bash time command, git for creating test changes, cmake --build for builds | **_Requirements:** 1, 2 | **Success:** Benchmark shows <5s incremental builds, <1s null builds, clear performance improvement documented | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts documenting benchmark results, then mark task as complete [x]

## Phase 2: Dependency Management (Conan 2.0)

- [x] 2.1. Create conanfile.txt with all dependencies
  - File: `conanfile.txt`
  - Define dependencies: Quill, Microsoft GSL, RapidCheck, Catch2, fmt
  - Pin exact versions for reproducibility
  - Configure generators for CMake integration
  - Purpose: Enable reproducible dependency management
  - _Leverage: Conan 2.0 syntax, ConanCenter packages
  - _Requirements: 3, 10_
  - _Prompt: **Role:** Build Engineer with Conan 2.0 expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create conanfile.txt with all required dependencies following requirements 3, 10. Pin exact versions: quill/4.1.0, ms-gsl/4.0.0, rapidcheck/cci.20230815, catch2/3.5.0, fmt/10.2.1. Configure CMakeToolchain generator. | **Restrictions:** Must use exact versions (not version ranges), must specify all required options, must work with CMake presets | **_Leverage:** Conan documentation, ConanCenter search, CMakeToolchain generator | **_Requirements:** 3, 10 | **Success:** conan install generates conan_toolchain.cmake, all dependencies fetch without errors, binary cache reduces build time | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 2.2. Integrate Conan toolchain with CMakePresets.json
  - File: `CMakePresets.json`
  - Add CMAKE_TOOLCHAIN_FILE pointing to conan_toolchain.cmake
  - Configure all presets to use Conan toolchain
  - Verify clean build workflow
  - Purpose: Enable deterministic builds with Conan dependencies
  - _Leverage: CMake toolchain file mechanism
  - _Requirements: 3, 7_
  - _Prompt: **Role:** Build Engineer with CMake and Conan integration expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Integrate Conan-generated toolchain into CMakePresets.json following requirements 3, 7. Set CMAKE_TOOLCHAIN_FILE to build/conan_toolchain.cmake in all presets. Verify workflow: conan install → cmake --preset → build. | **Restrictions:** Toolchain path must be relative to source dir, must work on all platforms, must handle missing toolchain gracefully | **_Leverage:** CMAKE_TOOLCHAIN_FILE variable, CMakePresets cacheVariables, Conan documentation | **_Requirements:** 3, 7 | **Success:** cmake --preset works after conan install, all dependencies found correctly, clean build completes successfully | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 2.3. Configure Conan binary caching
  - File: `docs/CONAN_SETUP.md` (new)
  - Document Conan remote configuration (ConanCenter)
  - Test local binary cache functionality
  - Document cache directory structure and cleanup
  - Purpose: Reduce clean build time from 10min to <2min
  - _Leverage: Conan remote add, local cache
  - _Requirements: 3_
  - _Prompt: **Role:** DevOps Engineer with Conan caching expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Configure Conan binary caching following requirement 3. Document remote setup, test local cache, verify clean build performance improvement. Create setup documentation. | **Restrictions:** Must work without custom remote (ConanCenter only initially), must document cache location, must provide cleanup instructions | **_Leverage:** conan remote add, conan cache path, ConanCenter public binaries | **_Requirements:** 3 | **Success:** Clean build with cache completes in <2min (vs 10min without), documentation explains cache setup, cache location documented | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 2.4. Update CMakeLists.txt to find Conan packages
  - File: `CMakeLists.txt`
  - Add find_package calls for all Conan dependencies
  - Link libraries to appropriate targets
  - Handle missing packages gracefully
  - Purpose: Enable usage of Conan-provided libraries
  - _Leverage: CMake find_package, target_link_libraries
  - _Requirements: 3_
  - _Prompt: **Role:** Build Engineer with CMake dependency management expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Update CMakeLists.txt to find and link Conan packages following requirement 3. Add find_package for quill, Microsoft.GSL, RapidCheck, Catch2. Link to appropriate targets. | **Restrictions:** Must use target_link_libraries with PRIVATE/PUBLIC correctly, must fail gracefully if package not found, must verify all headers found | **_Leverage:** find_package(quill REQUIRED), target_link_libraries, Conan package names | **_Requirements:** 3 | **Success:** All Conan packages found, libraries link correctly, build succeeds, can include headers (e.g., #include <quill/Quill.h>) | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 2.5. Test full dependency workflow
  - Test on clean machine (Linux and Windows)
  - Verify conan install → cmake → build workflow
  - Document any platform-specific issues
  - Purpose: Ensure reproducible builds across environments
  - _Leverage: CI environment or Docker
  - _Requirements: 3, 10_
  - _Prompt: **Role:** QA Engineer with cross-platform build testing expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Test complete Conan workflow on clean Linux and Windows machines following requirements 3, 10. Verify conan install fetches all dependencies, cmake configures correctly, build succeeds. Document any issues. | **Restrictions:** Must test on machines without cached dependencies, must test both platforms, must document exact commands used | **_Leverage:** Docker containers for clean Linux environment, fresh Windows VM or CI runner | **_Requirements:** 3, 10 | **Success:** Workflow works on clean machines, all steps documented, platform differences noted, troubleshooting guide created | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts documenting test results, then mark task as complete [x]

## Phase 3: Quill Logging Integration

- [x] 3.1. Create Quill logger wrapper
  - Files: `src/utils/logger.h`, `src/utils/logger.cpp`
  - Implement wrapper around Quill for YAMY-specific configuration
  - Configure RDTSC timestamping and JSON formatting
  - Provide LOG_INFO, LOG_ERROR, LOG_DEBUG macros
  - Purpose: Simplify Quill usage across codebase
  - _Leverage: Quill API, macro definitions
  - _Requirements: 4_
  - _Prompt: **Role:** C++ Developer with Quill logging expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create Quill logger wrapper in src/utils/logger.h/cpp following requirement 4. Configure RDTSC timestamps, JSON formatting, provide LOG_INFO/ERROR/DEBUG macros. Initialize backend thread in wrapper. | **Restrictions:** Wrapper must be simple to use, macros must support structured logging, must handle initialization/shutdown, must be thread-safe | **_Leverage:** quill::Logger, quill::Backend, quill::JsonFileHandler, macro variadic arguments | **_Requirements:** 4 | **Success:** Wrapper compiles and links, LOG_INFO macro works, JSON output verified, initialization/shutdown work correctly | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions, classes), then mark task as complete [x]

- [x] 3.2. Replace existing logging with Quill
  - Files: `src/core/engine/*.cpp`, `src/platform/**/*.cpp`
  - Find and replace all printf, std::cout, std::cerr calls
  - Use LOG_INFO/ERROR/DEBUG macros
  - Remove old logging infrastructure
  - Purpose: Migrate to zero-latency logging
  - _Leverage: grep for printf/cout, search/replace
  - _Requirements: 4_
  - _Prompt: **Role:** C++ Developer with refactoring expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Replace all existing logging (printf, std::cout, std::cerr) with Quill LOG_* macros following requirement 4. Search entire src/ directory, replace calls, remove old logging code. Verify compilation. | **Restrictions:** Must preserve log messages exactly, must use appropriate log level (INFO/ERROR/DEBUG), must not break compilation | **_Leverage:** grep -r "printf\\|std::cout\\|std::cerr" src/, search/replace in editor, LOG_INFO/ERROR/DEBUG macros | **_Requirements:** 4 | **Success:** All old logging replaced, no printf/cout/cerr in src/ directory, project compiles, logs output to JSON correctly | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 3.3. Configure JSON log output
  - File: `src/utils/logger.cpp`
  - Configure JsonFileHandler for Quill logger
  - Set log file location and rotation policy
  - Verify JSON structure for AI compatibility
  - Purpose: Enable AI-driven log analysis
  - _Leverage: Quill JsonFileHandler, file rotation
  - _Requirements: 4_
  - _Prompt: **Role:** C++ Developer with structured logging expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Configure Quill JSON output in logger wrapper following requirement 4. Use JsonFileHandler, set log file path to logs/yamy.json, configure rotation (10MB max). Verify JSON format includes timestamp, level, message, structured fields. | **Restrictions:** JSON must be valid (parseable by jq), rotation must work correctly, log directory must be created automatically | **_Leverage:** quill::JsonFileHandler, FileHandlerConfig for rotation, JSON validator | **_Requirements:** 4 | **Success:** Logs written to logs/yamy.json, JSON is valid, rotation works at 10MB, structured fields appear correctly | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 3.4. Benchmark logging performance
  - File: `tests/benchmark_logging.cpp`
  - Create microbenchmark for LOG_INFO latency
  - Measure hot-path overhead (99th percentile)
  - Compare with spdlog baseline
  - Purpose: Validate <1μs latency target
  - _Leverage: Catch2 BENCHMARK, RDTSC timing
  - _Requirements: 4_
  - _Prompt: **Role:** Performance Engineer with C++ benchmarking expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create logging performance benchmark following requirement 4. Measure LOG_INFO latency using RDTSC, calculate 99th percentile, compare with spdlog. Target <1μs on critical path. | **Restrictions:** Must measure hot path only (not backend thread), must use RDTSC for precision, must run enough iterations for statistical significance (10,000+) | **_Leverage:** RDTSC or std::chrono::high_resolution_clock, Catch2 BENCHMARK macro, statistical analysis | **_Requirements:** 4 | **Success:** Benchmark shows <1μs latency (99th percentile), comparison with spdlog included, results documented | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions, benchmarks), then mark task as complete [x]

- [x] 3.5. Document Quill usage guidelines
  - File: `docs/LOGGING_GUIDE.md` (new)
  - Document when to use each log level
  - Provide examples of structured logging
  - Explain critical path logging restrictions
  - Purpose: Ensure consistent logging practices
  - _Leverage: Quill documentation, examples
  - _Requirements: 4_
  - _Prompt: **Role:** Technical Writer with C++ logging expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create logging guidelines documentation following requirement 4. Explain log levels, structured logging syntax, critical path restrictions (no logging in input processing loop). Provide examples. | **Restrictions:** Keep concise (1-2 pages), focus on YAMY-specific patterns, include do's and don'ts, provide code examples | **_Leverage:** Quill documentation, existing YAMY code patterns | **_Requirements:** 4 | **Success:** Documentation is clear and actionable, examples compile, guidelines cover all common scenarios | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

## Phase 4: Contract Programming (Microsoft GSL)

- [x] 4.1. Add preconditions to core engine APIs
  - Files: `src/core/engine/engine.h`, `src/core/engine/engine.cpp`
  - Identify critical preconditions (e.g., key_code < MAX_KEYS)
  - Add Expects() macros to all public APIs
  - Document preconditions in Doxygen comments
  - Purpose: Catch caller bugs at entry points
  - _Leverage: Microsoft GSL Expects macro
  - _Requirements: 5_
  - _Prompt: **Role:** C++ Developer with contract programming expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add preconditions to core engine APIs following requirement 5. Identify critical preconditions (bounds checks, null checks), add Expects() macros, update Doxygen comments with @pre tags. | **Restrictions:** Focus on public APIs only (not internal functions), preconditions must be necessary (not overly strict), must document in comments | **_Leverage:** Microsoft GSL Expects(), @pre Doxygen tag, existing engine API documentation | **_Requirements:** 5 | **Success:** All public engine APIs have preconditions, Expects() compiles correctly, debug builds trap on violations | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 4.2. Add postconditions to critical functions
  - Files: `src/core/input/keymap.cpp`, `src/core/engine/engine_modifier.cpp`
  - Identify critical postconditions (e.g., result != nullptr)
  - Add Ensures() macros at function exit points
  - Test postcondition violations in debug builds
  - Purpose: Catch implementation bugs at exit points
  - _Leverage: Microsoft GSL Ensures macro
  - _Requirements: 5_
  - _Prompt: **Role:** C++ Developer with defensive programming expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add postconditions to critical functions following requirement 5. Identify postconditions for keymap lookup, modifier state tracking. Add Ensures() macros. Test violations in debug. | **Restrictions:** Postconditions must be verifiable, must not have side effects, must be at all return paths | **_Leverage:** Microsoft GSL Ensures(), multiple return paths handling | **_Requirements:** 5 | **Success:** Critical functions have postconditions, Ensures() traps violations in debug, release builds optimize out | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 4.3. Replace pointer+size with gsl::span
  - Files: `src/core/engine/engine_input.cpp`, `src/core/input/*.cpp`
  - Find functions using pointer+size parameters
  - Replace with gsl::span<const T> or gsl::span<T>
  - Update callers to use span
  - Purpose: Enable bounds-safe array access
  - _Leverage: gsl::span, gsl::make_span
  - _Requirements: 5_
  - _Prompt: **Role:** C++ Developer with modern C++ expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Replace pointer+size parameters with gsl::span following requirement 5. Search for functions taking (T* ptr, size_t size), replace with gsl::span<T>. Update callers to use gsl::make_span or direct construction. | **Restrictions:** Use const span where appropriate, don't break existing API unless necessary, update all callers | **_Leverage:** gsl::span, gsl::make_span, range-based for loops with span | **_Requirements:** 5 | **Success:** All array parameters use span, bounds checks in debug builds, code is cleaner and safer | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 4.4. Configure debug vs release contract behavior
  - File: `CMakeLists.txt`
  - Add compile definitions for GSL contract behavior
  - Debug: GSL_THROW_ON_CONTRACT_VIOLATION
  - Release: GSL_UNENFORCED_ON_CONTRACT_VIOLATION
  - Purpose: Zero-cost contracts in release builds
  - _Leverage: target_compile_definitions
  - _Requirements: 5_
  - _Prompt: **Role:** Build Engineer with C++ optimization expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Configure GSL contract behavior for debug/release builds following requirement 5. Set GSL_THROW_ON_CONTRACT_VIOLATION in debug, GSL_UNENFORCED_ON_CONTRACT_VIOLATION in release. Verify zero overhead in release. | **Restrictions:** Must be conditional on CMAKE_BUILD_TYPE, must apply to all targets using GSL, verify no runtime overhead in release | **_Leverage:** target_compile_definitions, CMAKE_BUILD_TYPE variable, benchmarks to verify zero cost | **_Requirements:** 5 | **Success:** Debug builds trap on contract violations, release builds have zero overhead, configuration verified in both modes | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 4.5. Document contract usage guidelines
  - File: `docs/CONTRACTS_GUIDE.md` (new)
  - Explain when to use Expects vs Ensures vs assert
  - Provide examples for common patterns
  - Document debug vs release behavior
  - Purpose: Ensure consistent contract usage
  - _Leverage: Microsoft GSL documentation
  - _Requirements: 5_
  - _Prompt: **Role:** Technical Writer with C++ contract programming expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create contract usage documentation following requirement 5. Explain Expects (preconditions), Ensures (postconditions), gsl::span usage. Provide examples. Document debug/release behavior. | **Restrictions:** Keep concise, focus on YAMY patterns, include do's and don'ts, explain when to use contracts vs exceptions | **_Leverage:** Microsoft GSL documentation, C++ Core Guidelines, existing YAMY code | **_Requirements:** 5 | **Success:** Documentation is clear, examples compile, covers all common scenarios, team understands when to use contracts | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

## Phase 5: Property-Based Testing (RapidCheck)

- [x] 5.1. Write keymap invariant properties
  - File: `tests/property_keymap.cpp`
  - Define 3 properties: lookup idempotence, define uniqueness, parent chain consistency
  - Integrate with Catch2 test suite
  - Run with 1000 iterations per property
  - Purpose: Explore keymap state space thoroughly
  - _Leverage: RapidCheck rc::check, Catch2 TEST_CASE
  - _Requirements: 6_
  - _Prompt: **Role:** QA Engineer with property-based testing expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Write RapidCheck properties for keymap invariants following requirement 6. Define properties: 1) lookup(key) is idempotent, 2) define(key, action) is unique, 3) parent chain is acyclic. Integrate with Catch2, run 1000 iterations. | **Restrictions:** Properties must be mathematically sound, must use RC_ASSERT, must integrate with existing test suite, must run in reasonable time | **_Leverage:** rc::check, rc::gen::arbitrary, Catch2 TEST_CASE wrapper, Keymap class API | **_Requirements:** 6 | **Success:** 3 properties defined and passing, RapidCheck finds and shrinks bugs (if any), tests run in <30s | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions, tests), then mark task as complete [x]

- [x] 5.2. Write modifier tracking properties
  - File: `tests/property_modifier.cpp`
  - Define 3 properties: all key-down have key-up, modifier state consistency, no stuck keys
  - Generate random input event sequences
  - Verify shrinking works on failures
  - Purpose: Test state machine correctness
  - _Leverage: RapidCheck generators, shrinking
  - _Requirements: 6_
  - _Prompt: **Role:** QA Engineer with state machine testing expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Write RapidCheck properties for modifier state tracking following requirement 6. Properties: 1) all key-down events have matching key-up, 2) modifier state is consistent, 3) no stuck keys after event sequence. Use rc::gen for input events. | **Restrictions:** Must generate realistic input sequences (not just random bytes), must verify state after all events processed, must test shrinking | **_Leverage:** rc::check, rc::gen::container, Engine::process(), state inspection APIs | **_Requirements:** 6 | **Success:** 3 properties defined, tests find edge cases, shrinking reduces failure to minimal sequence, documented example of shrunk case | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions, tests), then mark task as complete [x]

- [x] 5.3. Write layer switching properties
  - File: `tests/property_layer.cpp`
  - Define properties for layer activation/deactivation
  - Test layer stack invariants
  - Verify prefix keys don't leak
  - Purpose: Ensure layer switching correctness
  - _Leverage: RapidCheck, Engine layer API
  - _Requirements: 6_
  - _Prompt: **Role:** QA Engineer with keyboard remapper testing expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Write RapidCheck properties for layer switching following requirement 6. Properties for layer stack consistency, prefix key isolation, layer activation/deactivation. Generate sequences with layer-switching keys. | **Restrictions:** Must understand YAMY layer semantics, properties must capture actual requirements, must test error conditions | **_Leverage:** rc::check, Engine layer API, layer switching documentation | **_Requirements:** 6 | **Success:** Layer switching properties defined and passing, edge cases discovered (if any), comprehensive coverage of layer logic | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts (functions, tests), then mark task as complete [x]

- [x] 5.4. Configure CI to run property tests
  - File: `.github/workflows/ci.yml` or equivalent
  - Add property test step to CI pipeline
  - Configure 1000 iterations per property (5min max)
  - Set up nightly builds with 10,000 iterations
  - Purpose: Catch bugs automatically in CI
  - _Leverage: CI configuration, RapidCheck iteration control
  - _Requirements: 6_
  - _Prompt: **Role:** DevOps Engineer with CI/CD expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Configure CI to run property-based tests following requirement 6. Add test step running RapidCheck with 1000 iterations per property. Create nightly build with 10,000 iterations. Set timeout to 10min. | **Restrictions:** Must fail CI if property fails, must report which property failed, must run on all platforms, timeout must prevent infinite runs | **_Leverage:** CI test runner, ctest with timeout, RapidCheck iteration environment variable | **_Requirements:** 6 | **Success:** Property tests run in CI, failures fail the build, nightly tests run 10x more iterations, timeout works | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 5.5. Document property-based testing guide
  - File: `docs/PROPERTY_TESTING_GUIDE.md` (new)
  - Explain property-based testing concepts
  - Provide examples of good properties
  - Document how to interpret shrunk failures
  - Purpose: Enable team to write effective properties
  - _Leverage: RapidCheck documentation
  - _Requirements: 6_
  - _Prompt: **Role:** Technical Writer with property-based testing expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create property-based testing guide following requirement 6. Explain PBT concepts, provide examples of good vs bad properties, explain shrinking, document RapidCheck usage in YAMY. | **Restrictions:** Assume reader knows unit testing but not PBT, include concrete YAMY examples, explain shrinking with real example | **_Leverage:** RapidCheck documentation, QuickCheck papers, existing property tests | **_Requirements:** 6 | **Success:** Guide is clear for developers unfamiliar with PBT, examples are actionable, shrinking is explained with real YAMY example | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

## Phase 6: AI Compatibility

- [x] 6.1. Create docs/map.md codebase map
  - File: `docs/map.md`
  - List all major source files with brief descriptions
  - Organize by subsystem (engine, platform, input, etc.)
  - Include key constraints (e.g., "O(1) lookup", "<1ms latency")
  - Purpose: Enable AI agents to navigate codebase
  - _Leverage: Existing file structure
  - _Requirements: 8_
  - _Prompt: **Role:** Technical Writer with AI-driven development expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create codebase map at docs/map.md following requirement 8. List all major files organized by subsystem. Include: file path, brief description (1 sentence), key constraint or characteristic. Keep total <100 lines. | **Restrictions:** Be concise (max 1 sentence per file), organize logically, include only important files, optimize for AI context window | **_Leverage:** Existing directory structure, source file headers, architecture knowledge | **_Requirements:** 8 | **Success:** Map file is <100 lines, covers all major subsystems, AI agent can locate files in <3 queries (tested empirically) | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 6.2. Create .clinerules coding guidelines
  - File: `.clinerules`
  - Define architecture constraints (platform abstraction, no hot reload)
  - Specify coding style (PascalCase classes, camelCase functions, m_ prefix)
  - Document contract, logging, testing requirements
  - Purpose: Inject constraints into AI agent context
  - _Leverage: Existing coding standards
  - _Requirements: 8_
  - _Prompt: **Role:** Technical Lead with AI-assisted development expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create .clinerules file following requirement 8. Define architecture rules (platform abstraction, core engine platform-agnostic), coding style (naming, formatting), contract/logging/testing requirements. Format as AI-readable constraints. | **Restrictions:** Keep concise (1-2 pages), focus on enforceable rules, use imperative language ("MUST", "NEVER"), organize by category | **_Leverage:** Existing tech.md and structure.md, C++ Core Guidelines, YAMY-specific patterns | **_Requirements:** 8 | **Success:** Rules file is clear and concise, covers all critical constraints, AI agents follow rules when generating code | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 6.3. Create .cursorrules coding guidelines
  - File: `.cursorrules`
  - Shorter version of .clinerules for Cursor IDE
  - Focus on top 10 most important rules
  - Include quick reference (no hot reload, use Quill, use GSL)
  - Purpose: Support Cursor AI agent
  - _Leverage: .clinerules content
  - _Requirements: 8_
  - _Prompt: **Role:** Technical Writer with AI IDE expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create .cursorrules file following requirement 8. Condense .clinerules to top 10 rules. Format for Cursor IDE. Include quick reference section. Keep to 1 page. | **Restrictions:** Must be <1 page, prioritize most important rules, must be Cursor-compatible format, include "read docs/map.md first" instruction | **_Leverage:** .clinerules content, Cursor documentation, priority rules | **_Requirements:** 8 | **Success:** File is concise (<1 page), covers critical rules, Cursor IDE uses rules effectively | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 6.4. Update headers with Doxygen comments
  - Files: All headers in `src/core/`
  - Add Doxygen file and class comments
  - Document all public methods with @param, @return, @pre, @post
  - Include usage examples in @code blocks
  - Purpose: Make headers information-dense for AI
  - _Leverage: Doxygen syntax
  - _Requirements: 8_
  - _Prompt: **Role:** C++ Developer with documentation expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add comprehensive Doxygen comments to all core headers following requirement 8. File comments, class comments, method documentation with @param/@return/@pre/@post, usage examples. Focus on public APIs. | **Restrictions:** Only document public APIs (not private methods), be concise but complete, include examples for complex APIs, follow Doxygen best practices | **_Leverage:** Doxygen documentation, existing header comments, C++ documentation standards | **_Requirements:** 8 | **Success:** All public APIs documented, examples compile, Doxygen generates clean documentation, AI agents understand APIs from headers alone | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [x] 6.5. Remove redundant implementation comments
  - Files: All `.cpp` files in `src/core/`
  - Remove comments that just repeat code
  - Keep only "why" comments (not "what")
  - Preserve performance notes and edge case explanations
  - Purpose: Maximize signal-to-token ratio
  - _Leverage: Code review, refactoring
  - _Requirements: 8_
  - _Prompt: **Role:** C++ Developer with clean code expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Clean up implementation file comments following requirement 8. Remove redundant comments that repeat code. Keep only "why" comments, performance notes, edge cases. Improve code clarity where needed. | **Restrictions:** Don't remove useful comments (algorithm explanations, edge cases), improve code readability if removing comments makes it unclear, preserve TODO comments | **_Leverage:** Clean Code principles, self-documenting code practices | **_Requirements:** 8 | **Success:** Implementation files are cleaner, remaining comments are valuable, code is self-documenting, AI context window usage reduced | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

## Phase 7: Code Metrics Enforcement

- [x] 7.1. Install and configure lizard
  - Install lizard on development machines
  - Create CMake custom target for metrics check
  - Configure limits: 500 lines/file, 50 lines/function, CCN ≤15
  - Purpose: Enable automatic code quality checks
  - _Leverage: lizard Python tool, CMake add_custom_target
  - _Requirements: 9_
  - _Prompt: **Role:** Build Engineer with code quality tooling expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Install lizard and create CMake metrics check target following requirement 9. Configure limits: max 500 lines per file, 50 lines per function, cyclomatic complexity ≤15. Add check-metrics target. | **Restrictions:** Must work on Linux and Windows, must be optional (build works without lizard), must provide clear error messages on violations | **_Leverage:** lizard --length --CCN flags, CMake add_custom_target, find_program | **_Requirements:** 9 | **Success:** make check-metrics runs successfully, violations reported clearly, limits enforced correctly | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [ ] 7.2. Create pre-commit hook for metrics
  - File: `scripts/pre-commit-metrics.sh`
  - Create Git pre-commit hook running lizard
  - Check only staged files (not entire repo)
  - Provide option to bypass hook with --no-verify
  - Purpose: Catch violations before commit
  - _Leverage: Git hooks, lizard
  - _Requirements: 9_
  - _Prompt: **Role:** DevOps Engineer with Git hooks expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create pre-commit hook for code metrics following requirement 9. Run lizard on staged files only, reject commit on violations, allow bypass with --no-verify. Provide installation instructions. | **Restrictions:** Must check only staged files (not working directory), must be fast (<5s), must provide clear violation messages, must be installable | **_Leverage:** git diff --cached --name-only, lizard, Bash scripting | **_Requirements:** 9 | **Success:** Hook installs easily, checks staged files, rejects violations, bypass works, fast execution | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [ ] 7.3. Add metrics check to CI pipeline
  - File: `.github/workflows/ci.yml` or equivalent
  - Add step running make check-metrics
  - Fail build on violations
  - Report violations in CI logs
  - Purpose: Enforce metrics in code review
  - _Leverage: CI configuration
  - _Requirements: 9_
  - _Prompt: **Role:** DevOps Engineer with CI/CD expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Add code metrics check to CI pipeline following requirement 9. Run cmake --build --target check-metrics, fail build on violations, report violations clearly in logs. | **Restrictions:** Must run after build succeeds, must report all violations (not just first), must fail PR on violations, must be clear in logs | **_Leverage:** CI YAML syntax, CMake build targets, lizard output parsing | **_Requirements:** 9 | **Success:** CI runs metrics check, violations fail the build, violations reported clearly, blocks PR merges | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [ ] 7.4. Refactor violations in existing code
  - Identify files/functions exceeding limits
  - Refactor to meet metrics (split files/functions)
  - Preserve behavior (no functional changes)
  - Purpose: Bring existing code into compliance
  - _Leverage: Refactoring techniques
  - _Requirements: 9_
  - _Prompt: **Role:** C++ Developer with refactoring expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Refactor existing code to meet metrics following requirement 9. Find violations with lizard, split large files/functions, reduce complexity. Preserve exact behavior (no functional changes). | **Restrictions:** Must not change behavior (verify with tests), must not introduce new bugs, refactor incrementally, test after each change | **_Leverage:** Extract function, extract class refactorings, existing test suite | **_Requirements:** 9 | **Success:** All existing code meets metrics, tests still pass, behavior unchanged, code is cleaner | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [ ] 7.5. Document code metrics policy
  - File: `docs/CODE_METRICS.md` (new)
  - Explain rationale for each limit
  - Provide examples of good vs bad code
  - Document how to request exceptions
  - Purpose: Help team understand and follow metrics
  - _Leverage: Coding standards literature
  - _Requirements: 9_
  - _Prompt: **Role:** Technical Writer with software quality expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Document code metrics policy following requirement 9. Explain 500 lines/file, 50 lines/function, CCN ≤15 limits. Provide rationale, examples, exception process. | **Restrictions:** Keep concise (1-2 pages), provide concrete examples, explain why each limit matters, document exception process | **_Leverage:** Clean Code, Code Complete, existing YAMY patterns | **_Requirements:** 9 | **Success:** Policy is clear, rationale is convincing, examples are helpful, exception process defined | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

## Phase 8: Documentation and Validation

- [ ] 8.1. Create migration guide
  - File: `docs/MODERN_CPP_MIGRATION.md`
  - Document step-by-step migration process
  - Include installation instructions for all tools
  - Provide troubleshooting section
  - Purpose: Help developers adopt new toolchain
  - _Leverage: All previous work
  - _Requirements: All_
  - _Prompt: **Role:** Technical Writer with C++ development expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create comprehensive migration guide covering all requirements. Document tool installation (Mold, Conan, etc.), workflow changes (CMakePresets.json), new practices (Quill, GSL, RapidCheck). Include troubleshooting. | **Restrictions:** Be thorough but concise, provide copy-paste commands, cover all platforms, include common issues, link to detailed guides | **_Leverage:** All phase documentation, installation commands, troubleshooting from testing | **_Requirements:** All | **Success:** Guide is complete, developers can migrate successfully, all tools covered, troubleshooting helps with common issues | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [ ] 8.2. Create comprehensive benchmarks
  - File: `docs/PERFORMANCE_BENCHMARKS.md`
  - Benchmark build times (clean, incremental, null)
  - Benchmark logging latency (Quill vs spdlog)
  - Benchmark test coverage (unit vs property tests)
  - Purpose: Validate all performance targets
  - _Leverage: Benchmark results from all phases
  - _Requirements: 1, 2, 4, 6_
  - _Prompt: **Role:** Performance Engineer with benchmarking expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create comprehensive benchmark document following requirements 1, 2, 4, 6. Compile all benchmark results: build times, logging latency, test coverage. Compare baseline vs modern toolchain. Document methodology. | **Restrictions:** Use consistent hardware, document system specs, use statistical analysis (mean, 99th percentile), compare baseline vs target | **_Leverage:** Benchmark results from phases 1, 3, 5, statistical analysis tools | **_Requirements:** 1, 2, 4, 6 | **Success:** All targets validated, baseline vs modern comparison documented, methodology is reproducible, results are credible | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts documenting all benchmarks, then mark task as complete [x]

- [ ] 8.3. Create before/after comparison
  - File: `docs/TOOLCHAIN_COMPARISON.md`
  - Create side-by-side comparison table
  - Include: build system, linker, logging, contracts, testing, AI compatibility
  - Highlight improvements and trade-offs
  - Purpose: Summarize changes for stakeholders
  - _Leverage: All implementation work
  - _Requirements: All_
  - _Prompt: **Role:** Technical Writer with software architecture expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create before/after comparison document covering all requirements. Table format comparing: baseline (GCC+ld+Make) vs modern (Clang+Mold+Ninja+Conan+Quill+GSL+RapidCheck). Include improvements and trade-offs. | **Restrictions:** Be objective (show both pros and cons), use table format for clarity, include quantitative data where available | **_Leverage:** Requirements document, design decisions, benchmark results | **_Requirements:** All | **Success:** Comparison is clear and comprehensive, improvements quantified, trade-offs acknowledged, stakeholders understand changes | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

- [ ] 8.4. Test AI agent navigation
  - Test with Claude 3.7 and GPT-4o
  - Measure queries needed to locate files
  - Verify AI follows coding rules
  - Purpose: Validate AI compatibility
  - _Leverage: docs/map.md, .clinerules
  - _Requirements: 8_
  - _Prompt: **Role:** QA Engineer with AI-assisted development expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Test AI agent navigation following requirement 8. Ask Claude/GPT-4o to locate "input event processing code" without context. Measure queries needed. Verify AI follows .clinerules when generating code. Document results. | **Restrictions:** Don't provide context beyond docs/map.md initially, measure actual query count, test code generation compliance, document failures | **_Leverage:** Claude Code, GPT-4o with Cursor, structured testing methodology | **_Requirements:** 8 | **Success:** AI locates files in <3 queries, AI follows coding rules, documentation validated as AI-readable, results documented | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts documenting test results, then mark task as complete [x]

- [ ] 8.5. Create final validation report
  - File: `docs/TOOLCHAIN_VALIDATION_REPORT.md`
  - Validate all 10 requirements
  - Document test results for each acceptance criteria
  - List any deviations or exceptions
  - Purpose: Provide evidence of complete implementation
  - _Leverage: All testing and benchmarking
  - _Requirements: All_
  - _Prompt: **Role:** QA Lead with documentation expertise | **Task:** Implement the task for spec modern-cpp-toolchain, first run mcp__spec-workflow__spec-workflow-guide to get the workflow guide then implement the task: Create final validation report covering all requirements. For each requirement, document: acceptance criteria, test method, results, pass/fail. Include deviations, known issues, recommendations. | **Restrictions:** Be thorough and honest, document failures and workarounds, include evidence (benchmark results, screenshots), sign off on each requirement | **_Leverage:** All test results, benchmarks, CI logs, manual testing | **_Requirements:** All | **Success:** All requirements validated and documented, deviations explained, report is comprehensive and credible | **Instructions:** Before starting, mark this task as in-progress [-] in tasks.md. After completion, use log-implementation tool with detailed artifacts, then mark task as complete [x]

## Success Pattern Summary

This specification follows modern C++ best practices for 2024-2025:
- **Build Performance**: Mold/LLD linkers eliminate linking bottlenecks (<5s iteration)
- **Dependency Management**: Conan 2.0 ensures reproducible builds
- **Observability**: Quill provides zero-latency structured logging
- **Correctness**: Microsoft GSL contracts catch bugs at precondition checks
- **Testing**: RapidCheck explores state spaces that unit tests miss
- **AI Compatibility**: Structure optimized for AI agent navigation and code generation
- **Code Quality**: Automated metrics enforcement prevents technical debt

**Excluded**: Hot reload (cr/Live++) - architectural invasiveness outweighs benefits for YAMY.
