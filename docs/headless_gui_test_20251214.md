# Headless Daemon + yamy-gui Integration Test (2025-12-14)

## Environment
- Command root: `./build/bin` (CMake Makefiles)
- Display: `xvfb-run -a` for GUI smoke runs
- Daemon flag: `--no-restore`
- IPC socket: `/tmp/yamy-yamy-engine-1000` (control server `/tmp/yamy-engine.sock`)

## Run 1: Fresh daemon, GUI connect
- Daemon: `./build/bin/yamy --no-restore` → hooks 3 input devices, engine starts (`Engine started successfully`).
- GUI: `xvfb-run -a timeout 10s ./build/bin/yamy-gui --server-name yamy-engine`
- Observed in GUI log (`/tmp/yamy-gui-test.log`):
  - Connected to server, `CmdGetStatus` → `RspStatus engineRunning: true enabled: true`
  - `RspConfigList count 0` (no configs registered yet)

## Run 2: Daemon restart scenario
- Restarted daemon (`/tmp/yamy-daemon-test2.log`):
  - Hooking failed (`[ERROR] [input] Failed to hook any keyboard devices`) after previous grab.
- GUI: `xvfb-run -a timeout 8s ./build/bin/yamy-gui --server-name yamy-engine`
- Observed in GUI log (`/tmp/yamy-gui-test-restart.log`):
  - Connects and receives status, but `engineRunning: false` while `enabled: true`
  - Config list still empty; IPC channel remains responsive despite hook failure.

## Notes
- Ayatana AppIndicator libs detected; GUI build now succeeds with AppIndicator support gated on availability.
- Headless daemon can be restarted; input device hooks may fail if previous grab not fully released. Further cleanup/retry logic may be needed for repeated restarts.
