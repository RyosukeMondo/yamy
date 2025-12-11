# Requirements Document: YAMY v1.0 Release

## Introduction

This specification defines the requirements for releasing YAMY Linux v1.0, a production-ready cross-platform keyboard remapping utility. The current Linux port is 98.5% complete (129/131 tasks from the linux-complete-port spec), with only integration testing and documentation remaining. This spec addresses the final polish, quality assurance, and release preparation necessary for a stable v1.0 launch.

### Purpose

Transform the feature-complete Linux port into a production-ready release by:
- Completing pending integration tests and user documentation
- Fixing identified code quality issues and build errors
- Formalizing test coverage and CI/CD integration
- Validating compatibility across Linux distributions and desktop environments
- Preparing release artifacts and distribution channels

### Value to Users

- **Cross-Platform Developers**: Confidence that YAMY works reliably on Linux with same behavior as Windows
- **Power Users**: Comprehensive documentation enabling rapid onboarding and troubleshooting
- **Contributors**: Clean, well-tested codebase ready for community contributions
- **Package Maintainers**: Stable release with proper versioning and installation procedures

## Alignment with Product Vision

This release directly supports the **Short-Term (Q1 2025)** goals from product.md:

1. ✅ **Linux Feature Parity** - Core functionality complete (129/131 tasks)
2. 🔄 **Qt GUI Polish** - Final quality checks and fixes
3. 🔄 **Documentation** - User guides, troubleshooting, examples
4. ✅ **Community Building** - Ready for public release and adoption

### Success Metric Targets (from product.md)

| Metric | Target | Current Status | Gap |
|--------|--------|----------------|-----|
| **Performance** | <1ms latency (p99) | ✅ Achieved | None |
| **Quality** | <5 P0 bugs | ⚠️ 3 code issues + build errors | Fix 4 issues |
| **Test Coverage** | 80% minimum | ❓ Unknown | Add coverage reporting |
| **Documentation** | Complete user guides | ❌ Missing | Create 3 docs |

## Requirements

### FR-1: Complete Integration Testing

**User Story:** As a QA engineer, I want comprehensive integration tests covering all components working together, so that I can verify the system meets all functional requirements before release.

#### Acceptance Criteria

1. WHEN integration test suite executes THEN all 18 test cases pass without errors
2. WHEN build system compiles tests THEN no linker errors occur (ConfigWatcher vtable, IIPCChannel references)
3. WHEN tests run in CI environment THEN they complete successfully using Xvfb (no display required)
4. WHEN full lifecycle test runs THEN engine starts, loads config, processes keys, and stops cleanly
5. WHEN IPC tests execute THEN all 7+ command types (reload, stop, status, etc.) work correctly
6. WHEN GUI integration tests run THEN tray icon updates, dialogs open/close, notifications display
7. WHEN session tests execute THEN session save/restore maintains state correctly
8. WHEN performance tests run THEN key processing latency remains <1ms (p99)
9. WHEN memory tests execute THEN no memory leaks detected (valgrind/AddressSanitizer)

**Requirements Coverage:** NF-2 (Reliability), NF-6 (Testability)

---

### FR-2: Create End-User Documentation

**User Story:** As a new Linux user, I want comprehensive documentation explaining installation, configuration, and troubleshooting, so that I can successfully set up and use YAMY without prior knowledge.

#### Acceptance Criteria

1. WHEN user reads `docs/user-guide.md` THEN they understand installation, basic usage, dialogs, and preferences
2. WHEN user reads `docs/configuration-guide.md` THEN they can write complete .mayu files with key syntax, examples, and patterns
3. WHEN user encounters problems AND reads `docs/troubleshooting.md` THEN they find solutions to common issues, debugging steps, and FAQ
4. WHEN documentation includes screenshots THEN users can visually identify UI elements
5. WHEN examples are provided THEN all example configurations are tested and working
6. WHEN documentation is published THEN it follows project markdown standards with table of contents

**Requirements Coverage:** NF-3 (Usability), FR-8 (Help System)

---

### FR-3: Fix Build System Errors

**User Story:** As a developer, I want the build system to compile all targets without errors, so that I can build and test the application locally and in CI.

#### Acceptance Criteria

1. WHEN `cmake --build build --target yamy_regression_test` executes THEN compilation succeeds with zero errors
2. WHEN ConfigWatcher is instantiated THEN vtable references resolve correctly (no undefined references)
3. WHEN IIPCChannel null object is used THEN all virtual functions link properly
4. WHEN MOC (Qt Meta-Object Compiler) runs THEN Qt signals/slots are properly generated
5. WHEN CMakeLists.txt is updated THEN regression test target includes all necessary source files
6. WHEN build completes THEN executable `bin/yamy_regression_test` is created successfully

**Requirements Coverage:** NF-5 (Buildability), NF-6 (Testability)

---

### FR-4: Resolve Code Quality Issues

**User Story:** As a code reviewer, I want platform abstraction to be complete and thread-safe, so that the codebase maintains architectural integrity and prevents race conditions.

#### Acceptance Criteria

1. WHEN searching for `tstring` type usage THEN no typedef aliases exist (fully migrated to `std::string`)
2. WHEN examining `config_store.h` THEN Windows types (`BYTE`, `DWORD`) are replaced with `uint8_t`, `uint32_t`
3. WHEN multiple threads access `m_readerThreads` in `input_hook_linux.cpp` THEN mutex protection prevents race conditions
4. WHEN `interpretMetaCharacters()` function is analyzed THEN it is refactored into helper functions <50 lines each
5. WHEN `stringtool.cpp` is examined THEN file size is <500 lines (split into multiple files if needed)
6. WHEN all fixes are applied THEN code passes architectural review with zero violations

**Requirements Coverage:** NF-1 (Code Architecture), NF-4 (Maintainability)

---

### FR-5: Formalize Test Coverage

**User Story:** As a project maintainer, I want measurable test coverage metrics with CI integration, so that I can ensure code quality and prevent regressions.

#### Acceptance Criteria

1. WHEN coverage tools (lcov/gcov) run THEN code coverage percentage is calculated for all source files
2. WHEN coverage report is generated THEN HTML report shows per-file and per-function coverage
3. WHEN minimum coverage threshold is set THEN build fails if coverage drops below 80%
4. WHEN CI pipeline runs THEN coverage report is generated and archived as build artifact
5. WHEN performance benchmarks execute THEN latency measurements are logged and compared to baseline
6. WHEN coverage analysis completes THEN uncovered critical paths are identified and documented

**Requirements Coverage:** NF-2 (Reliability), NF-6 (Testability)

---

### FR-6: Validate Multi-Platform Compatibility

**User Story:** As a Linux user, I want YAMY to work reliably on my distribution and desktop environment, so that I don't encounter platform-specific bugs.

#### Acceptance Criteria

1. WHEN tested on Ubuntu 24.04 LTS (GNOME) THEN all features work without errors
2. WHEN tested on Fedora 40 (GNOME Wayland via XWayland) THEN input capture and injection function correctly
3. WHEN tested on Arch Linux (KDE Plasma) THEN Qt GUI integrates seamlessly with system theme
4. WHEN tested on Debian (XFCE) THEN system tray icon displays and context menu works
5. WHEN tested with multiple input methods (iBus, Fcitx) THEN no key conflicts occur
6. WHEN tested across platforms THEN configuration files are 100% portable between systems
7. WHEN compatibility matrix is complete THEN known limitations are documented per platform

**Requirements Coverage:** NF-2 (Reliability), NF-7 (Portability)

---

### FR-7: Prepare Release Artifacts

**User Story:** As a package maintainer, I want properly versioned binaries with installation scripts and documentation, so that I can distribute YAMY through package repositories.

#### Acceptance Criteria

1. WHEN version is set THEN `CMakeLists.txt`, source files, and documentation reflect v1.0.0
2. WHEN release builds execute THEN optimized binaries are generated for x86_64 architecture
3. WHEN installation script runs THEN binary, configs, and docs are placed in correct FHS locations
4. WHEN uninstall script runs THEN all installed files are cleanly removed
5. WHEN .deb package is built THEN it installs correctly on Ubuntu/Debian with proper dependencies
6. WHEN .rpm package is built THEN it installs correctly on Fedora/openSUSE with proper dependencies
7. WHEN PKGBUILD is created THEN AUR package installs correctly on Arch Linux
8. WHEN tarball is extracted THEN manual build instructions work on all target platforms

**Requirements Coverage:** NF-7 (Portability), NF-8 (Installability)

---

### FR-8: Create Release Notes and Changelog

**User Story:** As a user upgrading from a previous version, I want clear release notes explaining new features, bug fixes, and breaking changes, so that I can understand what changed.

#### Acceptance Criteria

1. WHEN `CHANGELOG.md` is read THEN all major features are listed with issue/PR references
2. WHEN `RELEASE-NOTES-1.0.md` is read THEN users understand highlights, installation steps, and upgrade path
3. WHEN release notes mention breaking changes THEN migration guide is provided
4. WHEN contributors are listed THEN all community contributions are acknowledged
5. WHEN license information is included THEN copyright year and MIT license text are current
6. WHEN release notes link to documentation THEN all links resolve correctly

**Requirements Coverage:** NF-3 (Usability), NF-4 (Maintainability)

---

### FR-9: Set Up CI/CD Pipeline

**User Story:** As a developer, I want automated builds, tests, and releases triggered by Git events, so that quality is enforced and releases are reproducible.

#### Acceptance Criteria

1. WHEN pull request is opened THEN GitHub Actions runs build, unit tests, and integration tests
2. WHEN tests fail THEN PR cannot be merged and author is notified
3. WHEN code coverage drops below 80% THEN build fails with coverage report
4. WHEN tag v1.0.0 is pushed THEN release workflow builds binaries for all architectures
5. WHEN release workflow completes THEN artifacts (.deb, .rpm, .tar.gz) are attached to GitHub release
6. WHEN CI cache is configured THEN build times are <5 minutes for incremental changes
7. WHEN CI runs on multiple platforms THEN matrix builds test Ubuntu, Fedora, and Arch

**Requirements Coverage:** NF-5 (Buildability), NF-6 (Testability)

---

## Non-Functional Requirements

### NF-1: Code Architecture and Modularity

- **Single Responsibility Principle**: Each file addresses single concern (violations fixed in FR-4)
- **Platform Abstraction**: Zero Windows-specific types in core engine (tstring removed)
- **Thread Safety**: All concurrent data structures protected by mutexes
- **SOLID Principles**: Interfaces segregated, dependencies injected, open/closed maintained

**Success Criteria:**
- Zero architectural violations in code review
- All files <500 lines (stringtool.cpp refactored)
- All functions <50 lines (interpretMetaCharacters refactored)
- Clean layer separation (core → platform interfaces → implementations)

---

### NF-2: Reliability

- **Crash-Free Operation**: Zero unhandled exceptions or segfaults in 1000+ hour soak test
- **Memory Safety**: Zero memory leaks detected by valgrind/AddressSanitizer
- **Graceful Degradation**: Missing dependencies (Qt, X11) result in clear error messages, not crashes
- **Data Integrity**: Configuration corruption does not prevent application start (fallback to default)

**Success Criteria:**
- Integration tests pass 100 consecutive runs without failure
- Performance benchmarks show <1ms latency sustained over 1 hour
- No crashes when X11 server disconnects or evdev device unplugged

---

### NF-3: Usability

- **Learning Curve**: New user can remap first key within 10 minutes of reading docs
- **Documentation Quality**: All common use cases have working examples
- **Error Messages**: All failures include actionable troubleshooting steps
- **Accessibility**: Keyboard-only navigation for all dialogs

**Success Criteria:**
- 5 beta testers complete setup without asking for help
- Documentation passes readability test (Flesch-Kincaid score >60)
- Zero ambiguous error messages in user testing

---

### NF-4: Maintainability

- **Code Coverage**: 80% minimum for core engine, 70% for platform layers
- **Documentation Coverage**: All public APIs have Doxygen comments
- **Commit Messages**: Follow conventional commit format
- **Code Review**: All PRs require approval and CI green status

**Success Criteria:**
- New contributor can fix simple bug within 2 hours of onboarding
- Technical debt items tracked in GitHub issues with severity labels
- Release process documented and reproducible

---

### NF-5: Buildability

- **Dependencies**: All dependencies available in standard package repositories
- **Build Time**: Clean build completes in <3 minutes on modern hardware
- **Toolchain**: Supports GCC 7+, Clang 8+, CMake 3.10+
- **Reproducible Builds**: Same source tarball produces bit-identical binaries

**Success Criteria:**
- Build succeeds on Ubuntu 20.04 LTS (oldest supported platform)
- Zero compiler warnings with `-Wall -Wextra` enabled
- Build instructions tested on fresh VM install

---

### NF-6: Testability

- **Unit Test Coverage**: All core components have unit tests (keymap, parser, engine)
- **Integration Test Coverage**: End-to-end scenarios tested (18 test cases)
- **CI Integration**: Tests run automatically on every commit
- **Mock/Stub Support**: All platform interfaces have test doubles

**Success Criteria:**
- Test suite runs in <2 minutes (unit + integration)
- Tests can run without display (Xvfb) for headless CI
- Flaky tests <1% failure rate

---

### NF-7: Portability

- **Distribution Compatibility**: Works on Ubuntu, Fedora, Arch, Debian
- **Desktop Environment**: Supports GNOME, KDE, XFCE, i3/Sway
- **Display Server**: X11 fully supported, Wayland via XWayland
- **Architecture**: x86_64 (primary), ARM64 (best-effort)

**Success Criteria:**
- Compatibility matrix validated across 3 distros × 2 DEs
- Zero platform-specific bugs in cross-platform code
- Configuration files portable between all platforms

---

### NF-8: Installability

- **Package Formats**: .deb, .rpm, AUR PKGBUILD, .tar.gz
- **FHS Compliance**: Binaries in `/usr/bin`, configs in `/etc/xdg`, docs in `/usr/share/doc`
- **Dependency Management**: All runtime dependencies declared in package metadata
- **Uninstall**: Clean removal with zero leftover files

**Success Criteria:**
- Package installation completes in <30 seconds
- Post-install scripts configure udev rules and group membership
- Uninstall removes 100% of installed files

---

## Out of Scope

The following are explicitly **NOT** included in v1.0 and deferred to future releases:

### Deferred to Future Releases
- ❌ **Wayland Native Support** - Requires compositor cooperation (Q2-Q3 2025)
- ❌ **Plugin System** - Lua scripting for custom actions (Q4 2025)
- ❌ **GUI Configuration Editor** - Visual keymap builder (Q2 2025)
- ❌ **Cloud Sync** - Configuration sharing across devices (Q2-Q3 2025)
- ❌ **macOS Port** - Cross-platform trinity completion (Q4 2025+)

### Explicitly Rejected
- ❌ **Mouse Remapping** - Out of project scope
- ❌ **Macro Recording** - Use scripting instead
- ❌ **Gaming Profiles** - Not target audience
- ❌ **Telemetry** - Privacy-first, local only

---

## Success Criteria Summary

**v1.0 Release is Ready When:**

1. ✅ All 18 integration tests pass (FR-1)
2. ✅ Three user documentation files complete (FR-2)
3. ✅ Build system compiles without errors (FR-3)
4. ✅ Three code quality issues resolved (FR-4)
5. ✅ Test coverage ≥80% with CI integration (FR-5)
6. ✅ Validated on Ubuntu, Fedora, Arch (FR-6)
7. ✅ Release artifacts built (.deb, .rpm, AUR) (FR-7)
8. ✅ Release notes and changelog complete (FR-8)
9. ✅ CI/CD pipeline operational (FR-9)
10. ✅ Zero P0 bugs, <5 P1 bugs in issue tracker

**Release Gates:**

- [ ] Security review completed (code signing, vulnerability scan)
- [ ] Legal review (license compliance, third-party notices)
- [ ] Performance benchmarks meet targets (<1ms, <5MB RAM, <1% CPU)
- [ ] Community beta testing (20+ testers, 2 weeks, zero critical bugs)
- [ ] Documentation reviewed by technical writer
- [ ] Package maintainers notified (AUR, Fedora Copr, Ubuntu PPA)

---

**Document Version**: 1.0
**Last Updated**: 2025-12-12
**Spec Name**: v1-0-release
**Reviewed By**: (Pending approval)
