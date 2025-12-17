# Tasks Document: Refactor E2E Test Infrastructure

- [x] 1. Create Framework Structure
  - **Action:** Create directories `tests/framework` and `tests/e2e`.
  - **Action:** Create empty `__init__.py` files.
  - _Requirements: 1.1_

- [x] 2. Implement ConfigParser
  - File: `tests/framework/parser.py`
  - **Implementation Detail:** Migrate logic from `MayuParser` in `automated_keymap_test.py`.
  - **Action:** Improve regex to be more robust (handle comments better).
  - _Requirements: 2.1_

- [x] 3. Implement EventInjector
  - File: `tests/framework/injector.py`
  - **Implementation Detail:** Extract `subprocess` logic for `yamy-test`.
  - **Action:** Ensure process lifecycle (start/stop) is robust.
  - _Requirements: 1.1, 4.1_

- [ ] 4. Implement LogMonitor
  - File: `tests/framework/monitor.py`
  - **Implementation Detail:** Use `evdev` to capture output from "Yamy Virtual" device.
  - **Action:** Implement timeout logic for event capture.
  - _Requirements: 1.1_

- [ ] 5. Implement ReportGenerator
  - File: `tests/framework/reporter.py`
  - **Implementation Detail:** Create a class that stores `TestResult` objects.
  - **Action:** Implement `save_json` method.
  - _Requirements: 3.2_

- [ ] 6. Create Main Runner
  - File: `tests/e2e/test_runner.py`
  - **Implementation Detail:** Stitch components together.
  - **Action:** CLI argument parsing (`argparse`).
  - _Requirements: Non-functional Usability_
