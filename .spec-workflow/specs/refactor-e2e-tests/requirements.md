# Requirements Document: Refactor E2E Test Infrastructure

## Introduction

The current E2E test infrastructure (`tests/automated_keymap_test.py` and `yamy-test`) is functional but monolithic and brittle. It relies on simplified regex parsing of configuration files and lacks standard reporting formats. This project aims to modularize the Python test runner and improve the fidelity of test case generation.

## Alignment with Product Vision

Robust automated testing is essential for maintaining the stability of the keyboard remapping engine, especially as we refactor the core logic. A reliable, modular test suite allows for safer changes and faster regression detection.

## Requirements

### Requirement 1: Modular Architecture

**User Story:** As a developer, I want to easily add new test strategies or output formats without modifying the core runner logic.

#### Acceptance Criteria
1.  **Component Separation:** The test runner SHALL be separated into distinct classes: `ConfigParser`, `EventInjector`, `LogMonitor`, `TestRunner`, and `ReportGenerator`.
2.  **Interfaces:** Clear interfaces (abstract base classes) SHALL be defined for each component.

### Requirement 2: Improved Config Parsing

**User Story:** As a QA engineer, I want the test runner to understand complex `.mayu` constructs so I don't have to manually verify edge cases.

#### Acceptance Criteria
1.  **Robust Parsing:** The Python-based config parser SHALL be improved to handle more than just simple `def subst`. It should parse `mod` definitions and basic `keyseq` assignments if possible, or at least fail gracefully.
2.  **AST Alignment:** Ideally, the Python parser structures should mirror the C++ `ConfigAST` to facilitate future integration (e.g., Python bindings to C++ parser).

### Requirement 3: Standardized Reporting

**User Story:** As a release manager, I want test results in a standard format so I can integrate them into CI dashboards.

#### Acceptance Criteria
1.  **JUnit XML:** The runner SHALL be capable of generating a JUnit-compatible XML report (`TEST-yamy-e2e.xml`).
2.  **JSON Report:** A detailed JSON report including all events sent and received SHALL be generated for debugging.

### Requirement 4: Headless Execution

**User Story:** As a CI pipeline, I want to run tests without a physical display or user session.

#### Acceptance Criteria
1.  **No Display Dependency:** The tests SHALL run successfully in a headless environment (using `uinput` for input and verifying logs/evdev output).
2.  **Sudo/Permissions:** The runner SHALL check for necessary permissions (uinput access) and fail with a clear message if missing.

## Non-Functional Requirements

### Usability
- **CLI:** The runner SHALL accept standard arguments (`--config`, `--output`, `--verbose`).
- **Setup:** A `requirements.txt` SHALL be provided for Python dependencies.
