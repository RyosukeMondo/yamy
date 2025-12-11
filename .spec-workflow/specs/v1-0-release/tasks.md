# Tasks Document: YAMY v1.0 Release

## Overview

The linux-complete-port spec is now **100% complete** (131/131 tasks). This v1.0 release spec focuses exclusively on **production readiness**: fixing build errors, resolving code quality issues, formalizing QA processes, and preparing release infrastructure.

## Track 1: Fix Build System (3 tasks)

### Core Issue
Integration tests exist but cannot compile due to:
- ConfigWatcher MOC (Qt Meta-Object Compiler) not generated
- IIPCChannel vtable undefined references
- Missing Qt5Test linkage

---

- [x] 1.1 Fix ConfigWatcher MOC generation in CMakeLists.txt
  - File: CMakeLists.txt
  - Add `qt5_wrap_cpp()` for ConfigWatcher header to generate MOC files
  - Include generated MOC sources in yamy_regression_test target
  - Purpose: Resolve "undefined reference to vtable for ConfigWatcher" linker error
  - _Leverage: Existing Qt5 MOC setup for other Qt classes_
  - _Requirements: FR-3_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Build engineer with CMake and Qt expertise | Task: Fix ConfigWatcher MOC generation - Step 1: Locate ConfigWatcher class definition in src/core/settings/config_watcher.h, Step 2: In CMakeLists.txt find Qt5 MOC generation section (search for qt5_wrap_cpp), Step 3: Add config_watcher.h to MOC generation list: qt5_wrap_cpp(CONFIG_WATCHER_MOC src/core/settings/config_watcher.h), Step 4: Add generated MOC files to yamy_regression_test target sources, Step 5: Rebuild and verify no vtable errors | Restrictions: Don't modify ConfigWatcher class itself, Follow existing MOC pattern from other Qt classes, Ensure MOC runs before compilation | Success: cmake --build build --target yamy_regression_test completes without "vtable for ConfigWatcher" errors, ConfigWatcher signals/slots work correctly, Regression tests link successfully | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.2 Implement IIPCChannel vtable (add implementation file)
  - File: src/core/platform/ipc_interface.cpp (NEW)
  - Create implementation file for IIPCChannel interface providing vtable definition
  - Add destructor implementation (`IIPCChannel::~IIPCChannel() = default;`)
  - Purpose: Resolve "undefined reference to vtable for IIPCChannel" linker error
  - _Leverage: Existing interface pattern from IWindowSystem, IInputHook_
  - _Requirements: FR-3_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: C++ developer with expertise in abstract interfaces and vtables | Task: Provide vtable implementation for IIPCChannel - Step 1: Create new file src/core/platform/ipc_interface.cpp, Step 2: Include header #include "ipc_interface.h", Step 3: Add namespace yamy::platform scope, Step 4: Implement virtual destructor: IIPCChannel::~IIPCChannel() = default;, Step 5: Add file to CMakeLists.txt PLATFORM_INTERFACE_SOURCES, Step 6: Rebuild and verify linker error resolved | Restrictions: Do NOT modify interface header, Keep implementation minimal (single destructor only), Follow existing pattern from other interfaces | Success: Linker no longer reports "undefined reference to vtable for IIPCChannel", IPCChannelNull test double compiles without errors, All IPC tests link successfully | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 1.3 Link Qt5Test module for test executable
  - File: CMakeLists.txt
  - Add `find_package(Qt5Test REQUIRED)` and link Qt5::Test to yamy_regression_test
  - Purpose: Enable QSignalSpy and Qt test utilities in integration tests
  - _Leverage: Existing Qt5 package discovery for Qt5::Widgets, Qt5::Gui_
  - _Requirements: FR-3_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: CMake build specialist | Task: Link Qt5Test module to test executable - Step 1: In CMakeLists.txt find existing find_package(Qt5 COMPONENTS ...) call, Step 2: Add Test to COMPONENTS list, Step 3: Locate target_link_libraries for yamy_regression_test, Step 4: Add Qt5::Test to PRIVATE dependencies, Step 5: Build and verify no Qt test utility linking errors | Restrictions: Only add to test targets not main executable, Don't add unnecessary Qt modules, Follow existing Qt linkage pattern | Success: cmake finds Qt5Test successfully, yamy_regression_test links Qt5::Test, QSignalSpy and QTest utilities available in tests, No Qt-related linking errors | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

---

## Track 2: Fix Code Quality Issues (4 tasks)

### Core Issues
- tstring typedef still exists (should be fully removed)
- config_store.h uses Windows types (BYTE/DWORD)
- input_hook_linux.cpp lacks mutex for m_readerThreads vector
- stringtool.cpp violates file size limit (569 lines, limit 500)

---

- [x] 2.1 Remove tstring typedef and migrate to std::string
  - Files: src/utils/stringtool.h (delete typedef), ~100 files (replace usage)
  - Search entire codebase for `tstring` usage and replace with `std::string`
  - Remove `using tstring = std::string;` typedef from stringtool.h
  - Purpose: Complete platform abstraction by eliminating Windows-era type alias
  - _Leverage: Previous refactoring pattern from Windows types to platform types_
  - _Requirements: FR-4, NF-1_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Refactoring specialist with regex/sed expertise | Task: Remove tstring typedef across codebase - Step 1: Search all tstring usages: grep -r "\\btstring\\b" src/ --include="*.h" --include="*.cpp", Step 2: Replace with std::string using sed: find src/ -name "*.cpp" -o -name "*.h" | xargs sed -i 's/\\btstring\\b/std::string/g', Step 3: Delete typedef line from src/utils/stringtool.h:31, Step 4: Rebuild entire project and fix any compilation errors, Step 5: Run unit tests to verify no behavioral changes | Restrictions: Do NOT modify string literals or comments containing "tstring", Preserve Unicode handling (wstring separate), Test after each batch of changes | Success: Zero occurrences of tstring in source files (grep returns empty), Full rebuild succeeds with no errors, All unit tests pass, No functionality changes | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [x] 2.2 Replace Windows types in config_store.h with stdint types
  - Files: src/utils/config_store.h, src/platform/windows/config_store_win32.cpp, src/platform/linux/config_store_linux.cpp
  - Replace `BYTE` with `uint8_t` and `DWORD` with `uint32_t` in interface and implementations
  - Update all callers of read()/write() methods
  - Purpose: Remove Windows-specific types from platform-agnostic interface
  - _Leverage: Existing stdint usage in other platform abstractions_
  - _Requirements: FR-4, NF-1_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Platform abstraction developer | Task: Replace Windows types with standard integer types - Step 1: In config_store.h replace all BYTE with uint8_t, DWORD with uint32_t, Step 2: Add #include <cstdint> if not present, Step 3: Update config_store_win32.cpp implementations (cast to Windows types internally if needed), Step 4: Update config_store_linux.cpp implementations (no changes likely needed), Step 5: Find all callers of read()/write() methods and update variable types, Step 6: Rebuild and verify no type mismatch warnings | Restrictions: Interface must remain binary compatible, Don't change method signatures (parameter order/count), Keep Windows implementation functional | Success: No BYTE or DWORD types in config_store.h, Windows build compiles without warnings, Linux build compiles without warnings, Config read/write tests pass on both platforms | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.3 Add mutex protection for m_readerThreads vector in input_hook_linux.cpp
  - File: src/platform/linux/input_hook_linux.cpp
  - Add `std::mutex m_readerThreadsMutex` member variable
  - Protect all vector operations (push_back, clear, iteration, size) with std::lock_guard
  - Purpose: Prevent race conditions during concurrent device hotplug events
  - _Leverage: Existing mutex usage in other platform components_
  - _Requirements: FR-4, NF-2_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Multithreading expert with C++ concurrency knowledge | Task: Add thread-safe vector access - Step 1: Locate m_readerThreads vector in input_hook_linux.cpp class, Step 2: Add private member std::mutex m_readerThreadsMutex;, Step 3: Find all m_readerThreads accesses (push_back, clear, iteration, size, empty), Step 4: Wrap each access with std::lock_guard<std::mutex> lock(m_readerThreadsMutex);, Step 5: Test with thread sanitizer: cmake -DCMAKE_CXX_FLAGS=-fsanitize=thread, Step 6: Verify no data races during device plug/unplug | Restrictions: Minimize critical section duration (don't hold lock during long operations), Use lock_guard not manual lock/unlock, Don't introduce deadlocks | Success: Thread sanitizer reports zero data races, Device hotplug stress test passes (plug/unplug 100 times), No crashes or hangs during concurrent access, Code review confirms proper lock usage | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 2.4 Refactor stringtool.cpp to meet file size limit (<500 lines)
  - Files: src/utils/stringtool.cpp (split), stringtool_conversion.cpp (NEW), stringtool_parsing.cpp (NEW)
  - Extract interpretMetaCharacters() and helpers into stringtool_parsing.cpp
  - Extract UTF conversion functions into stringtool_conversion.cpp
  - Keep core string utilities in stringtool.cpp
  - Purpose: Meet code quality KPI (max 500 lines per file) and improve maintainability
  - _Leverage: Existing module structure and private header pattern_
  - _Requirements: FR-4, NF-1, NF-4_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Refactoring architect with C++ modularity expertise | Task: Split stringtool.cpp into focused modules - Step 1: Analyze stringtool.cpp identify three logical groups: (a) core string ops, (b) encoding conversion, (c) parsing/escapes, Step 2: Create stringtool_parsing.cpp with parseEscapeSequence(), parseOctalNumber(), parseHexNumber(), interpretMetaCharacters(), Step 3: Create stringtool_conversion.cpp with UTF-8/UTF-16 conversion functions, Step 4: Keep remaining functions in stringtool.cpp (~200 lines), Step 5: Create stringtool_private.h for shared internal helpers if needed, Step 6: Update CMakeLists.txt add new .cpp files to UTILS_SOURCES, Step 7: Refactor interpretMetaCharacters() break into helper functions <50 lines each, Step 8: Rebuild and run string utility tests verify no behavioral changes | Restrictions: Do NOT change public API in stringtool.h, Keep test interface identical, Ensure no circular dependencies, All files must be <500 lines | Success: stringtool.cpp <200 lines, stringtool_conversion.cpp <150 lines, stringtool_parsing.cpp <150 lines, No function >50 lines, All existing tests pass without modification, Zero functionality changes | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

---

## Track 3: Formalize Test Coverage (3 tasks)

### Goal
Measure code coverage, enforce 80% threshold, integrate with CI

---

- [ ] 3.1 Add CMake coverage target with lcov/gcov
  - File: CMakeLists.txt
  - Add `ENABLE_COVERAGE` option with `--coverage` compiler/linker flags
  - Create `coverage` custom target that runs tests and generates HTML report
  - Purpose: Enable local coverage measurement for developers
  - _Leverage: Existing CTest integration and Google Test suite_
  - _Requirements: FR-5, NF-6_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Build engineer with gcov/lcov expertise | Task: Add CMake coverage infrastructure - Step 1: Add option(ENABLE_COVERAGE "Enable code coverage" OFF), Step 2: Add conditional compiler flags: if(ENABLE_COVERAGE) add_compile_options(--coverage -O0 -g) add_link_options(--coverage) endif(), Step 3: Find lcov and genhtml programs: find_program(LCOV lcov REQUIRED), Step 4: Create custom target coverage that: (a) zeroes counters, (b) runs CTest, (c) captures coverage with lcov, (d) removes /usr/* and tests/* from report, (e) generates HTML with genhtml, Step 5: Add coverage threshold check: grep coverage % and fail if <80%, Step 6: Document usage in README: cmake -DENABLE_COVERAGE=ON && cmake --build build --target coverage | Restrictions: Coverage target only available when ENABLE_COVERAGE=ON, Don't slow down normal builds, Exclude test code and external dependencies from coverage | Success: cmake --build build --target coverage generates build/coverage/html/index.html, Coverage report shows per-file line coverage, Threshold check fails build if <80%, Clean coverage data between runs | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 3.2 Generate baseline coverage report and identify gaps
  - Command: `cmake -B build -DENABLE_COVERAGE=ON && cmake --build build --target coverage`
  - Analyze HTML report to find uncovered critical paths
  - Document current coverage percentage and gaps in GitHub issue
  - Purpose: Establish baseline and prioritize additional test coverage
  - _Leverage: Newly created coverage target from task 3.1_
  - _Requirements: FR-5, NF-6_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA engineer with coverage analysis experience | Task: Generate baseline coverage report - Step 1: Run cmake -B build -DENABLE_COVERAGE=ON -DBUILD_REGRESSION_TESTS=ON, Step 2: Build coverage target: cmake --build build --target coverage, Step 3: Open build/coverage/html/index.html in browser, Step 4: Extract overall coverage percentage from report, Step 5: Identify top 10 files with lowest coverage, Step 6: Create GitHub issue titled "Test Coverage Baseline - v1.0" with: (a) overall coverage %, (b) per-module breakdown, (c) critical uncovered functions, (d) prioritized list of gaps, Step 7: If coverage <80% create additional tasks to add tests | Restrictions: This is analysis only no code changes, Focus on critical paths (engine, platform layer) not utilities, Don't aim for 100% (diminishing returns) | Success: Coverage report generated successfully, Baseline documented in GitHub issue with metrics, Gaps identified and prioritized, If coverage <80% plan created to close gap | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 3.3 Integrate coverage reporting with GitHub Actions CI
  - File: .github/workflows/ci.yml
  - Add coverage build job that uploads report to Codecov or GitHub artifacts
  - Configure PR comments with coverage delta
  - Purpose: Automate coverage enforcement and provide visibility to reviewers
  - _Leverage: Existing CMake coverage target and GitHub Actions setup_
  - _Requirements: FR-5, FR-9, NF-6_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: DevOps engineer with CI/CD and GitHub Actions expertise | Task: Integrate coverage in CI pipeline - Step 1: Create or modify .github/workflows/ci.yml, Step 2: Add job build-with-coverage: runs-on ubuntu-latest, steps: checkout, install dependencies (lcov), cmake with -DENABLE_COVERAGE=ON, build coverage target, Step 3: Upload coverage report as artifact: uses actions/upload-artifact@v3 with path build/coverage/, Step 4: Add codecov upload: uses codecov/codecov-action@v3 with files build/coverage/coverage.info (optional but recommended), Step 5: Configure PR status check to fail if coverage drops >2% | Restrictions: Only run coverage on PRs and main branch (not every commit), Cache dependencies to speed up builds, Don't upload coverage for forks (security), Use matrix strategy if testing multiple platforms | Success: PR checks show coverage build job, Coverage report artifact downloadable from Actions tab, Codecov comment appears on PR (if configured), Build fails if coverage threshold not met, CI runs complete in <10 minutes | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

---

## Track 4: Platform Compatibility Testing (3 tasks)

### Goal
Validate YAMY works across major Linux distributions and desktop environments

---

- [ ] 4.1 Test on Ubuntu 24.04 LTS with GNOME (Wayland via XWayland)
  - Environment: Ubuntu 24.04, GNOME 46, XWayland
  - Install from .deb package and run functional test checklist
  - Measure performance benchmarks (latency, memory, CPU)
  - Purpose: Validate on most popular Linux desktop distribution
  - _Leverage: Integration test suite and yamy --benchmark flag_
  - _Requirements: FR-6, NF-2, NF-7_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA tester with Ubuntu and GNOME experience | Task: Validate on Ubuntu 24.04 - Step 1: Provision Ubuntu 24.04 VM or container with GNOME desktop, Step 2: Build .deb package: cpack -G DEB, Step 3: Install: sudo dpkg -i yamy_*.deb, Step 4: Run functional checklist: (a) System tray icon displays, (b) Settings dialog opens, (c) Key remapping A→B works, (d) Ctrl+A remap works, (e) Config reload works, (f) Session persists across reboot, Step 5: Run performance benchmark: yamy --benchmark --duration=60 > ubuntu-bench.log, Step 6: Verify metrics: (a) p99 latency <1ms, (b) Memory <10MB, (c) CPU idle <1%, Step 7: Document results and any issues in test report | Restrictions: Test on clean VM (no existing yamy config), Use standard GNOME (not modified), Test both X11 session and Wayland session, Capture logs for any failures | Success: All functional tests pass, Performance metrics meet targets, No crashes or errors in 1 hour soak test, Installation/uninstallation clean, Test report documents Ubuntu compatibility | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 4.2 Test on Fedora 40 with GNOME Wayland (native Wayland)
  - Environment: Fedora 40, GNOME 46 Wayland, XWayland fallback for YAMY
  - Install from .rpm package and verify XWayland compatibility
  - Document any Wayland-specific quirks or limitations
  - Purpose: Validate on Wayland-first distribution
  - _Leverage: Fedora package build (cpack -G RPM)_
  - _Requirements: FR-6, NF-7_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA tester with Fedora and Wayland experience | Task: Validate on Fedora Wayland - Step 1: Provision Fedora 40 VM with GNOME Wayland (default), Step 2: Build .rpm package: cpack -G RPM, Step 3: Install: sudo dnf install yamy_*.rpm, Step 4: Verify YAMY runs via XWayland: echo $WAYLAND_DISPLAY should be set, check /proc/$(pgrep yamy)/environ for DISPLAY=:1, Step 5: Run functional checklist (same as Ubuntu), Step 6: Test Wayland-specific scenarios: (a) Window focus across Wayland and XWayland apps, (b) Key remapping in native Wayland apps (should work via XWayland), (c) Clipboard integration, Step 7: Document limitations: native Wayland capture not supported (expected), Step 8: Verify no security warnings or compositor errors | Restrictions: Accept XWayland dependency (native Wayland out of scope for v1.0), Document any compositor-specific issues, Test only default GNOME settings | Success: Installation succeeds with correct dependencies, YAMY works via XWayland in Wayland session, All functional tests pass, Known limitations documented, No compositor crashes or errors | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 4.3 Test on Arch Linux with KDE Plasma
  - Environment: Arch Linux, KDE Plasma 6, X11
  - Install from source (simulate AUR installation) and test Qt integration
  - Verify system tray and theme integration with KDE
  - Purpose: Validate on rolling release with KDE desktop environment
  - _Leverage: PKGBUILD for AUR distribution_
  - _Requirements: FR-6, NF-7_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA tester with Arch Linux and KDE experience | Task: Validate on Arch/KDE - Step 1: Provision Arch Linux VM with KDE Plasma desktop, Step 2: Create PKGBUILD following Arch packaging standards (reference task 5.3), Step 3: Build from source: makepkg -si, Step 4: Verify Qt5 theme integration: yamy should match Breeze theme colors, icons, Step 5: Test KDE-specific features: (a) System tray icon in Plasma panel, (b) Right-click context menu style, (c) Notifications use Plasma notification daemon, (d) Settings dialog fonts match KDE, Step 6: Run functional checklist (same as Ubuntu), Step 7: Test Plasma-specific windows: (a) Key remap in Konsole, (b) Window matching for Dolphin, Kate, Step 8: Document any KDE-specific configuration needed | Restrictions: Test on current Arch (rolling release), Use official Qt5 packages (not Qt6), Verify dependencies available in official repos | Success: Build from source succeeds with standard Arch toolchain, Qt GUI integrates seamlessly with Breeze theme, System tray icon displays correctly in Plasma panel, All functional tests pass, No KDE-specific issues found | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

---

## Track 5: Release Preparation (5 tasks)

### Goal
Create distribution packages, release notes, and automate release process

---

- [ ] 5.1 Create Debian/Ubuntu .deb package with CPack
  - Files: CMakeLists.txt (add CPack configuration), DEBIAN control files
  - Configure CPack to generate .deb with correct dependencies and FHS layout
  - Add postinst script for udev rules and group setup
  - Purpose: Enable installation on Debian-based distributions via dpkg
  - _Leverage: Existing CMake install rules_
  - _Requirements: FR-7, NF-8_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Debian packaging expert with CPack experience | Task: Create .deb package - Step 1: In CMakeLists.txt add CPack configuration: set(CPACK_GENERATOR "DEB"), set(CPACK_DEBIAN_PACKAGE_NAME "yamy"), set version/maintainer/description, Step 2: Specify dependencies: set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5widgets5, libqt5gui5, libx11-6, libudev1"), Step 3: Set install layout: binaries to /usr/bin, configs to /etc/yamy, docs to /usr/share/doc/yamy, icons to /usr/share/icons, Step 4: Create packaging/postinst script: (a) Add user to input group if not member, (b) Copy udev rules if needed, (c) Echo instructions to log out/in, Step 5: Add postinst to CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA, Step 6: Test package build: cpack -G DEB, Step 7: Test installation: sudo dpkg -i yamy_*.deb, verify files in correct locations | Restrictions: Follow Debian policy (FHS compliance), Don't require user interaction during install, Handle upgrade scenario (don't fail if already installed), Clean uninstall (postrm script) | Success: cpack generates yamy_1.0.0-1_amd64.deb, dpkg -i installs without errors, Files placed in FHS-compliant locations, postinst adds user to input group, dpkg -r removes cleanly, lintian reports no critical errors | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.2 Create Fedora/SUSE .rpm package with CPack
  - Files: CMakeLists.txt (add RPM CPack config), yamy.spec template
  - Configure CPack to generate .rpm with correct dependencies
  - Handle Fedora-specific paths and SELinux if needed
  - Purpose: Enable installation on RPM-based distributions
  - _Leverage: .deb package structure from task 5.1_
  - _Requirements: FR-7, NF-8_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: RPM packaging specialist | Task: Create .rpm package - Step 1: In CMakeLists.txt add RPM generator: list(APPEND CPACK_GENERATOR "RPM"), Step 2: Configure RPM-specific settings: CPACK_RPM_PACKAGE_LICENSE "MIT", CPACK_RPM_PACKAGE_GROUP "Applications/System", CPACK_RPM_PACKAGE_REQUIRES "qt5-qtbase, libX11, libudev", Step 3: Set release number: CPACK_RPM_PACKAGE_RELEASE "1%{?dist}", Step 4: Create %post scriptlet for group membership (same logic as .deb postinst), Step 5: Test build: cpack -G RPM, Step 6: Test on Fedora VM: sudo dnf install yamy_*.rpm, Step 7: Verify RPM query: rpm -ql yamy shows correct file list | Restrictions: Support Fedora and openSUSE (don't hardcode Fedora-only paths), Handle SELinux contexts if needed, Sign RPM for production release | Success: cpack generates yamy-1.0.0-1.fc40.x86_64.rpm, dnf install succeeds on Fedora 40, Files installed in correct locations, rpm -V yamy verifies package integrity, dnf remove cleans up completely | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.3 Create Arch Linux PKGBUILD for AUR
  - File: packaging/PKGBUILD (NEW)
  - Write PKGBUILD that builds from source tarball
  - Test with makepkg and namcap validation
  - Purpose: Enable Arch users to install via AUR (yay, paru)
  - _Leverage: CMake build system and install rules_
  - _Requirements: FR-7, NF-8_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Arch Linux packager with AUR experience | Task: Create PKGBUILD - Step 1: Create packaging/PKGBUILD file, Step 2: Set package metadata: pkgname=yamy, pkgver=1.0.0, pkgrel=1, pkgdesc="Cross-platform keyboard remapper", arch=('x86_64'), url, license=('MIT'), Step 3: Specify dependencies: depends=('qt5-base' 'libx11' 'libudev'), makedepends=('cmake' 'gcc'), Step 4: Write build() function: cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr, cmake --build build, Step 5: Write package() function: DESTDIR="$pkgdir" cmake --install build, Step 6: Add source URL and checksums: source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz"), sha256sums=('SKIP' for testing), Step 7: Test build: makepkg -si, Step 8: Validate: namcap PKGBUILD, namcap yamy-*.pkg.tar.zst | Restrictions: Follow Arch packaging guidelines, Use official repos dependencies only, Don't bundle libraries, Provide .install file if post-install messages needed | Success: makepkg builds package successfully, namcap reports no errors, sudo pacman -U installs correctly, Files in FHS-compliant locations, Package can be submitted to AUR | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.4 Write CHANGELOG.md and RELEASE-NOTES-1.0.md
  - Files: CHANGELOG.md (NEW), RELEASE-NOTES-1.0.md (NEW)
  - Document all changes since last version (or initial release)
  - Write user-facing release notes highlighting key features
  - Purpose: Inform users what's new and how to upgrade
  - _Leverage: Git commit history and closed GitHub issues_
  - _Requirements: FR-8, NF-3_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Technical writer with semantic versioning knowledge | Task: Write release documentation - Step 1: Create CHANGELOG.md following Keep a Changelog format: sections for Added, Changed, Fixed, Removed, Step 2: Extract changes from git log since project start (or previous release), Step 3: Group commits by category: Features (131 tasks from linux-complete-port), Bug Fixes (code quality issues), Documentation (user guides), Step 4: Reference GitHub issues/PRs for each significant change: [#123], Step 5: Create RELEASE-NOTES-1.0.md with: (a) Release highlights (5 bullet points), (b) Installation instructions (Ubuntu, Fedora, Arch), (c) Upgrade notes (n/a for initial release), (d) Known issues (link to GitHub), (e) Contributors list (git shortlog -sn), (f) Link to full CHANGELOG, Step 6: Add Breaking Changes section (none for v1.0), Step 7: Proofread for clarity and grammar | Restrictions: Focus on user-visible changes (not internal refactoring), Use present tense ("Adds" not "Added"), Include attribution for community contributions, Keep release notes <1 page (detailed changelog separate) | Success: CHANGELOG.md follows standard format, All significant changes documented, RELEASE-NOTES-1.0.md is concise and user-friendly, Links to issues/PRs work, Contributors acknowledged, Both files reviewed and approved | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 5.5 Set up GitHub Actions release workflow
  - File: .github/workflows/release.yml (NEW)
  - Automate release builds triggered by version tags (v1.0.0)
  - Upload packages (.deb, .rpm, .tar.gz) to GitHub Releases
  - Purpose: Automate and standardize release process
  - _Leverage: Existing CI workflow and CPack configuration_
  - _Requirements: FR-9, NF-5_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: DevOps automation engineer | Task: Create automated release pipeline - Step 1: Create .github/workflows/release.yml, Step 2: Trigger on tags: on push tags 'v*', Step 3: Create job matrix for Ubuntu and Fedora builders, Step 4: Build steps: (a) checkout code, (b) install dependencies, (c) cmake -DCMAKE_BUILD_TYPE=Release, (d) build, (e) cpack -G DEB (Ubuntu) or -G RPM (Fedora), Step 5: Create source tarball: make package_source, Step 6: Upload artifacts: actions/upload-artifact for build verification, Step 7: Create GitHub release: softprops/action-gh-release with files *.deb *.rpm *.tar.gz, body_path RELEASE-NOTES-1.0.md, Step 8: Test workflow: git tag v1.0.0-rc1 && git push --tags | Restrictions: Only trigger on annotated tags (not lightweight), Verify tag matches version in CMakeLists.txt, Sign packages if GPG key available, Don't upload debug builds | Success: Push tag v1.0.0 triggers workflow, Workflow builds on Ubuntu and Fedora runners, Packages generated successfully (.deb and .rpm), GitHub Release created with all artifacts attached, Release notes displayed on release page, Workflow completes in <15 minutes | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

---

## Track 6: Final Validation (2 tasks)

### Goal
Verify all requirements met, no regressions, ready for release

---

- [ ] 6.1 Run full integration test suite on all platforms
  - Command: `xvfb-run -a ./build/bin/yamy_regression_test`
  - Execute on Ubuntu, Fedora, Arch after all fixes applied
  - Verify zero test failures and no memory leaks
  - Purpose: Final validation that all components work together
  - _Leverage: Fixed integration tests from Track 1_
  - _Requirements: All FR requirements, NF-2 (Reliability)_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: QA validation engineer | Task: Execute full test suite - Step 1: On Ubuntu 24.04 build with tests: cmake -B build -DBUILD_REGRESSION_TESTS=ON, cmake --build build, Step 2: Run tests with Xvfb (headless): xvfb-run -a ./build/bin/yamy_regression_test --gtest_output=xml:test-results.xml, Step 3: Verify all 18 integration tests pass (100% pass rate), Step 4: Run with AddressSanitizer: cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" && rebuild && run tests, verify no leaks, Step 5: Repeat on Fedora 40 VM: same build and test procedure, Step 6: Repeat on Arch VM: same build and test procedure, Step 7: Collect test results XML and logs from all platforms, Step 8: Document any platform-specific failures or warnings | Restrictions: Tests must pass on all platforms (no platform-specific skips), Use latest stable versions (Ubuntu 24.04, Fedora 40, current Arch), No flaky tests (rerun 3 times, 100% pass rate), Zero memory leaks tolerated | Success: All 18 integration tests pass on Ubuntu/Fedora/Arch, AddressSanitizer reports zero leaks, No segfaults or crashes, Test results XML shows 18/18 passed, Execution time <5 minutes per platform | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

- [ ] 6.2 Verify all v1.0 release criteria met
  - Checklist: Review requirements document and validate each criterion
  - Create GitHub milestone "v1.0" and close all associated issues
  - Get sign-off from maintainers/stakeholders
  - Purpose: Formal gate before release tag
  - _Leverage: Requirements document FR-1 through FR-9_
  - _Requirements: All requirements_
  - _Prompt: Implement the task for spec v1-0-release, first run spec-workflow-guide to get the workflow guide then implement the task: Role: Release manager | Task: Validate release readiness - Step 1: Create checklist from requirements.md Success Criteria section, Step 2: Verify each item: (a) All 18 integration tests pass [FR-1], (b) Three user docs complete [FR-2], (c) Build system compiles without errors [FR-3], (d) Code quality issues resolved [FR-4], (e) Coverage ≥80% [FR-5], (f) Tested on 3 platforms [FR-6], (g) Packages built [FR-7], (h) Release notes written [FR-8], (i) CI/CD pipeline working [FR-9], Step 3: Run performance benchmarks verify <1ms latency, <5MB RAM, <1% CPU, Step 4: Create GitHub milestone v1.0 if not exists, Step 5: Review all open issues tag critical blockers move non-blockers to v1.1, Step 6: Close milestone when all blockers resolved, Step 7: Get approval from maintainers post summary in GitHub discussion, Step 8: If all checks pass proceed to release (task 5.5 workflow) | Restrictions: No shortcuts (all criteria must be met), Document any exceptions with justification, Don't release with known P0 bugs | Success: All release criteria verified and documented, GitHub milestone shows 0 open issues, Performance benchmarks meet targets, Maintainer approval received, v1.0 tag ready to push | After completion: 1) Mark task as in-progress [-] in tasks.md before starting, 2) Log implementation using log-implementation tool with detailed artifacts, 3) Mark task as complete [x] in tasks.md_

---

## Task Execution Guidelines

### Execution Order
Tasks are organized into sequential tracks but can be parallelized within constraints:
- **Track 1 (Build Fixes)** must complete before **Track 6 (Validation)**
- **Track 2 (Code Quality)** independent, can run parallel to Track 1
- **Track 3 (Coverage)** depends on Track 1 (tests must build)
- **Track 4 (Platform Tests)** depends on Track 1 and 2 (stable build)
- **Track 5 (Release Prep)** can start anytime, finishes last
- **Track 6 (Validation)** runs last, depends on all previous tracks

### Parallel Execution
Within each track, tasks can execute in parallel by multiple agents if no dependencies:
- Track 1: Sequential (1.1 → 1.2 → 1.3, each fixes part of build)
- Track 2: Parallel (2.1, 2.2, 2.3 independent; 2.4 can overlap)
- Track 3: Sequential (3.1 → 3.2 → 3.3)
- Track 4: Fully parallel (4.1, 4.2, 4.3 independent VMs)
- Track 5: Mostly parallel (5.1/5.2/5.3 independent; 5.4 anytime; 5.5 needs 5.1/5.2/5.3)
- Track 6: Sequential (6.1 → 6.2)

### Task Completion Workflow
For each task:
1. Update tasks.md: Change `[ ]` to `[-]` (in-progress)
2. Read the _Prompt field for detailed guidance
3. Implement according to task description
4. Test implementation thoroughly
5. **CRITICAL: Log implementation** using log-implementation tool with:
   - Clear summary of what was implemented
   - Files modified/created with line counts
   - **Required artifacts** (APIs, components, functions, classes, integrations)
6. Update tasks.md: Change `[-]` to `[x]` (completed)

### Success Metrics
- All 17 tasks completed and marked [x]
- Zero P0 bugs in issue tracker
- Performance: <1ms latency (p99), <10MB RAM, <1% CPU idle
- Coverage: ≥80% line coverage
- Compatibility: Ubuntu, Fedora, Arch all passing
- Release artifacts generated and uploaded

---

**Document Version**: 1.0
**Last Updated**: 2025-12-12
**Spec Name**: v1-0-release
**Total Tasks**: 17 (across 6 tracks)
**Estimated Effort**: 60-80 hours (1-2 weeks with dedicated focus)
**Reviewed By**: (Pending approval)
