# YAMY GUI Test Report (2025-12-14)

## Environment
- Build artifacts: `build/bin` (CMake Makefiles), Qt 5 runtime available.
- Display: `xvfb-run -a` for GUI invocations.
- IPC server: default logical name `yamy-engine` (daemon accessible on `/tmp/yamy-engine.sock` during runs).
- Host keyboard layout: jp (affects evdev keycode expectations).
- Permissions: non-root; some input devices are busy/ungrabbed.

## Executed Tests & Results
- `./build/bin/yamy_ipc_client_gui_test --gtest_color=no` → **PASS (6.0s)**. Drives `yamy_mock_ipc_server`, sends GetStatus/SetEnabled/SwitchConfig/ReloadConfig, verifies emitted signals and disconnect handling.
- `xvfb-run -a ./build/bin/yamy-gui --server-name yamy-engine` (killed after 4s) → **PASS**. Connection + status handshake logged by 0.161s after launch; `RspStatus engineRunning=false enabled=true`, `RspConfigList count 0`.
- `./build/bin/yamy_linux_test` (full gtest suite, 265 cases) → **FAIL / TIMED OUT (120s)**. Keycode mapping suites fail across letter/number/function/modifier cases because evdev codes differ on jp layout; later `InputHookLinuxCallbackTest.InstallFailsWithoutDevices` blocked on grabbing `/dev/input/event*` (device busy) and hit the harness timeout. No remaining suites executed after timeout.
- Prior manual integration (see `docs/headless_gui_test_20251214.md`) → **MIXED**. Run 1: GUI connected to headless daemon, `engineRunning=true`, empty config list. Run 2 after daemon restart: IPC responsive but `engineRunning=false` due to device grab failure.
- Historical debug (task 1.4) → **FAIL (legacy)**: daemon accepted connections but returned no GUI (`0x51xx`) responses to CmdGetStatus/SetEnabled/SwitchConfig/ReloadConfig. Subsequent runs now return responses, so this is resolved.

## Performance Metrics
- GUI cold-start to first IPC responses: ~0.16s from process launch to `RspStatus`/`RspConfigList` receipt (timed with stdout timestamps during 4s bounded run under `xvfb-run`).
- GUI smoke dwell: ran for 5s window (`timeout 5s xvfb-run ...`); remained connected and responsive with no crashes.
- IPC mock integration test runtime: ~6.0s wall clock including mock server spawn/teardown.

## Coverage Summary
- IPC client ↔ mock server command/response flow: covered by `yamy_ipc_client_gui_test`.
- GUI launch + IPC handshake: covered by timed `xvfb-run` smoke launch.
- Daemon + GUI end-to-end: covered by `docs/headless_gui_test_20251214.md` (two runs: fresh daemon and restart scenario).
- Daemon/system tests: `yamy_linux_test` exercised window queries, key mappings, input hook install paths; mapping suites failing under current host layout.

## Known Issues / Limitations
- Keycode mapping tests fail on jp layout; expected evdev codes in `input_injector_linux_test` assume US mappings. Requires layout-aware expectations or running on US layout for green runs.
- Input hook tests and daemon restarts can fail when keyboard devices are already grabbed (`Device or resource busy`), leading to `engineRunning=false` even though IPC stays responsive.
- Config list empty in current environment because no configs are registered; GUI handles but feature validation is limited.
- Harness-timed runs (`yamy_linux_test`, bounded GUI smoke) exit with non-zero due to enforced timeout, not crashes.

## Artifacts / Logs
- Mock IPC integration test output: `./build/bin/yamy_ipc_client_gui_test --gtest_color=no`.
- GUI launch timing logs (stdout) captured during `xvfb-run -a ./build/bin/yamy-gui --server-name yamy-engine` (bounded to 4s).
- Headless daemon + GUI integration notes: `docs/headless_gui_test_20251214.md`.
- Failed system test excerpts (keycode mapping + input hook): `./build/bin/yamy_linux_test` output (jp layout, device busy).
