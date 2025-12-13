# Qt Engine Integration - Manual Test Guide

## Prerequisites

```bash
# 1. Build the project
cd /home/rmondo/repos/yamy
mkdir -p build_release && cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4 yamy yamy-ctl

# 2. Verify binaries exist
ls -lh bin/yamy bin/yamy-ctl

# 3. Add yourself to the input group (required for keyboard access)
sudo usermod -a -G input $USER
# Then logout and login again for group change to take effect
```

## Test Plan

### Test 1: Basic IPC Commands (No GUI)

```bash
# Clean start
pkill -9 yamy 2>/dev/null
rm -f /tmp/yamy-engine.sock

# Start yamy in background with no session restore
./bin/yamy --no-restore &
sleep 2

# Test status command
./bin/yamy-ctl status
# Expected: Engine: Stopped | Config: (none) | Uptime: Xs | Keys: 0 | Keymap: default

# Test config command
./bin/yamy-ctl config
# Expected: {"config_path":"","config_name":"","loaded_time":"..."}

# Test metrics command
./bin/yamy-ctl metrics
# Expected: {"latency_avg_ns":0,"latency_p99_ns":0,"latency_max_ns":0,"cpu_usage_percent":0.0,"keys_per_second":0.0}

# Test keymaps command
./bin/yamy-ctl keymaps
# Expected: {"keymaps":[]}

# Kill yamy
pkill yamy
```

**Expected Result:** All commands should return JSON data without errors.

---

### Test 2: Configuration Loading via IPC

First, create a minimal test configuration:

```bash
# Create a simple test config
cat > /tmp/test.mayu <<'EOF'
# Minimal test configuration
keymap Global
    # Simple test mapping: Caps Lock -> Escape
    mod ctrl += CapsLock
    key CapsLock = Escape
EOF
```

Now test loading it:

```bash
# Start yamy
./bin/yamy --no-restore &
sleep 2

# Load the test configuration
./bin/yamy-ctl reload /tmp/test.mayu

# Check config was loaded
./bin/yamy-ctl config
# Expected: Shows "test.mayu" in config_path

# Check status
./bin/yamy-ctl status
# Expected: Config should show "/tmp/test.mayu"

# Check keymaps
./bin/yamy-ctl keymaps
# Expected: Should show "Global" keymap

# Kill yamy
pkill yamy
```

**Expected Result:** Config should load without errors, commands should show the loaded config.

---

### Test 3: Engine Start/Stop

```bash
# Start yamy and load config
./bin/yamy --no-restore &
sleep 2
./bin/yamy-ctl reload /tmp/test.mayu

# Start the engine
./bin/yamy-ctl start
sleep 1

# Check status
./bin/yamy-ctl status
# Expected: State should be "running"

# Stop the engine
./bin/yamy-ctl stop
sleep 1

# Check status again
./bin/yamy-ctl status
# Expected: State should be "stopped"

# Kill yamy
pkill yamy
```

**Expected Result:** Engine should start and stop on command, status should reflect current state.

---

### Test 4: GUI Testing (System Tray)

```bash
# Start yamy (this time with GUI)
./bin/yamy &
```

**Manual Steps:**

1. **Check System Tray:**
   - Ubuntu GNOME: Install `gnome-shell-extension-appindicator` if not already installed
   - Look for yamy icon in system tray (top-right corner or hidden icons)
   - Icon should appear (may show enabled/disabled state)

2. **Right-click Tray Icon:**
   - Should see menu with options: Settings, About, Quit

3. **Open Settings Dialog:**
   - Click "Settings" from tray menu
   - Settings window should open

4. **Load Configuration:**
   - In Settings dialog, click "Browse" or "Load Config"
   - Navigate to `/home/rmondo/repos/yamy/keymaps/master.mayu`
   - Click "Load" or "OK"
   - Should see notification: "Configuration loaded successfully"

5. **Check Status:**
   ```bash
   ./bin/yamy-ctl status
   # Should show master.mayu as active config
   ```

6. **Test Notifications:**
   - Load a valid config → Should see success notification
   - Try to load invalid file → Should see error notification

**Expected Result:** GUI should be responsive, config loading should show notifications, tray icon should update.

---

### Test 5: Keyboard Remapping (Real Functionality)

This tests if the real engine actually remaps keys.

**Setup:**
```bash
# Make sure you're in the input group
groups | grep input
# If not shown, run: sudo usermod -a -G input $USER
# Then logout/login

# Start yamy
./bin/yamy &
sleep 2

# Load a simple test config
cat > /tmp/simple-remap.mayu <<'EOF'
# Map CapsLock to Escape
keymap Global
    key CapsLock = Escape
EOF

./bin/yamy-ctl reload /tmp/simple-remap.mayu
./bin/yamy-ctl start
```

**Test Steps:**

1. Open a text editor (gedit, kate, vim, etc.)
2. Press **CapsLock** key
3. Expected: Should produce **Escape** key behavior (exit insert mode in vim, etc.)
4. If it works, the engine is successfully remapping keys!

```bash
# Check metrics after pressing some keys
./bin/yamy-ctl metrics
# Should show non-zero latency and keys_per_second
```

**Expected Result:** CapsLock should behave as Escape, metrics should show key processing.

---

### Test 6: Session Persistence

```bash
# Start yamy and load config
./bin/yamy &
sleep 2
./bin/yamy-ctl reload /tmp/test.mayu
./bin/yamy-ctl start

# Check session file was created
cat ~/.config/yamy/session.json
# Should show:
# - activeConfigPath: "/tmp/test.mayu"
# - engineWasRunning: true

# Kill yamy
pkill yamy
sleep 1

# Start yamy again WITHOUT --no-restore
./bin/yamy &
sleep 3

# Check if config was restored
./bin/yamy-ctl status
# Expected: Should show /tmp/test.mayu as active config
# Expected: State should be "running" (engine auto-started)

pkill yamy
```

**Expected Result:** Session should be saved and restored across restarts.

---

### Test 7: Error Handling

```bash
# Start yamy
./bin/yamy --no-restore &
sleep 2

# Try to load non-existent file
./bin/yamy-ctl reload /nonexistent/file.mayu
# Expected: Error message "Failed to load configuration" or "File not found"

# Try to load invalid config
cat > /tmp/invalid.mayu <<'EOF'
this is not valid mayu syntax @#$%
EOF

./bin/yamy-ctl reload /tmp/invalid.mayu
# Expected: Error message about parse failure

# Check yamy didn't crash
./bin/yamy-ctl status
# Expected: Should still respond (yamy still running)

pkill yamy
```

**Expected Result:** Errors should be handled gracefully without crashing.

---

### Test 8: Memory Leak Check (Optional)

Requires `valgrind`:

```bash
# Install valgrind if needed
sudo apt-get install valgrind

# Run yamy under valgrind (takes longer to start)
valgrind --leak-check=full --track-origins=yes ./bin/yamy --no-restore 2>&1 | tee /tmp/valgrind.log &
sleep 5

# Do some operations
./bin/yamy-ctl reload /tmp/test.mayu
./bin/yamy-ctl start
sleep 2
./bin/yamy-ctl stop
sleep 1

# Quit yamy cleanly
pkill -INT yamy
sleep 3

# Check valgrind output
grep "definitely lost" /tmp/valgrind.log
grep "indirectly lost" /tmp/valgrind.log

# Expected: "0 bytes in 0 blocks" for both (or very small amounts)
```

**Expected Result:** No significant memory leaks detected.

---

## Success Criteria

✅ All IPC commands return valid JSON without errors
✅ Configuration files load successfully
✅ Engine starts and stops on command
✅ GUI shows tray icon and responds to clicks
✅ Notifications appear for config load/errors
✅ **Keyboard keys are actually remapped** (THE BIG ONE!)
✅ Session persists across restarts
✅ Errors are handled gracefully
✅ No memory leaks detected

---

## Troubleshooting

**Problem: Permission denied errors**
```bash
# Add yourself to input group
sudo usermod -a -G input $USER
# Logout and login again
```

**Problem: No tray icon visible**
```bash
# Install AppIndicator extension for GNOME
sudo apt-get install gnome-shell-extension-appindicator
gnome-extensions enable ubuntu-appindicators@ubuntu.com
```

**Problem: Config not loading**
```bash
# Check debug log
tail -f /tmp/yamy-debug.log
```

**Problem: IPC commands hang**
```bash
# Check if socket exists
ls -la /tmp/yamy-engine.sock

# Check if yamy is running
ps aux | grep yamy

# Kill and restart
pkill -9 yamy
./bin/yamy --no-restore &
```

---

## Next Steps After Testing

If all tests pass, you're ready to:
1. Install yamy system-wide: `sudo make install`
2. Create startup script for auto-launch
3. Document any remaining issues
4. Celebrate the successful integration! 🎉
