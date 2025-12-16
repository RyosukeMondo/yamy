# YAMY GUI User Guide

This guide covers the new Linux flow that pairs the headless **yamy** daemon with the **yamy-gui** front-end. It explains the architecture, how to run both processes, core UI interactions, and common troubleshooting steps.

## Architecture at a Glance
- **Daemon (`yamy`)**: Headless Qt Core process that owns the engine, keyboard hooks, and IPC control server (default socket: `/tmp/yamy-engine.sock`, advertised as `yamy-engine`).
- **GUI (`yamy-gui`)**: Qt Widgets client that talks to the daemon via IPC (CmdGetStatus/CmdSetEnabled/CmdSwitchConfig/CmdReloadConfig).
- **Auto-reconnect**: GUI retries on disconnect with backoff; manual reconnect is available from the File menu.

```
Keyboard devices → yamy (daemon, headless) ⇄ IPC (/tmp/yamy-engine.sock) ⇄ yamy-gui (Qt UI)
```

## Quick Start (Linux)
1. **Build** (release example):
   ```bash
   cmake -B build_release -DCMAKE_BUILD_TYPE=Release -DBUILD_LINUX_STUB=ON -DBUILD_QT_GUI=ON
   cmake --build build_release -j$(nproc)
   ```
2. **Start the daemon** (headless):
   ```bash
   ./build_release/bin/yamy --no-restore
   ```
   - Needs permission to grab keyboards (usually `input` group membership).
   - Optional verbose IPC logs: `YAMY_DEBUG_IPC=1 ./build_release/bin/yamy --no-restore`.
3. **Launch the GUI**:
   ```bash
   ./build_release/bin/yamy-gui              # connects to default server name yamy-engine
   ./build_release/bin/yamy-gui --server-name yamy-engine-alt   # if you override the server name
   ```
   - For headless CI/smoke checks: `xvfb-run -a timeout 10s ./build_release/bin/yamy-gui`.

## UI Tour
- **Connection + status row**: Colored indicator (green = connected, red = disconnected). Text shows connection state; warning dialogs surface daemon errors.
- **Enable/Disable toggle**: Mirrors daemon enable state and sends `CmdSetEnabled` to change it.
- **Configuration row**: Active config label, config dropdown (populated from `RspConfigList`), and **Reload** button for `CmdReloadConfig`.
- **Menu bar**:
  - *File*: Manual reconnect, Quit.
  - *Tools*: Settings, Preferences, Logs, Investigate, Manage Configurations, Notification History.
  - *Help*: Keyboard Shortcuts, Configuration Examples, About.

## Common Workflows
- **Connect to daemon**: Start `yamy` first, then `yamy-gui`. If the daemon restarts, the GUI auto-reconnects; use *File → Reconnect* for an immediate retry.
- **Switch configuration**: Choose an entry in the config dropdown (populated from the daemon), wait for the status message to confirm the switch.
- **Reload configuration**: Press **Reload** to force the active config to re-parse. The UI disables controls during the reload and re-enables after the response.
- **Inspect logs and events**: Use *Tools → Logs* to review recent activity or *Tools → Investigate* to run interactive checks with the engine.
- **View keyboard shortcuts/examples**: *Help → Keyboard Shortcuts* and *Help → Configuration Examples* list built-in bindings and sample `.mayu` snippets.

## Troubleshooting
- **GUI cannot connect**: Ensure `yamy` is running and listening on `/tmp/yamy-engine.sock`. If using a custom server name, pass `--server-name` to both daemon and GUI. Toggle `YAMY_DEBUG_IPC=1` for verbose frames or use `tools/debug_ipc_communication.sh` to watch traffic.
- **Empty config list**: The daemon exposes configs it has loaded; add or register configs via the Config Manager dialog or verify your `.mayu` files are readable by the daemon.
- **Engine disabled or error dialog**: Error details come from the daemon’s `lastError`. Common causes include missing input permissions or lingering device grabs after a crash—restart the daemon cleanly and re-run.
- **Reload fails**: The active config path may be invalid or contain parse errors. Fix the `.mayu` file, then retry **Reload**; check the Logs dialog for parse failures.
- **Daemon restart issues**: If repeated restarts fail to hook keyboards, release device grabs (log out/in or stop other grabbers) and start `yamy` again before launching the GUI.

## Screenshots
![Menu bar and status](menu-ja.png)
![Investigation dialog](investigate-ja.png)
