# YAMY Linux UAT Status Report

**Date:** 2025-12-14
**Environment:** Linux 6.14.0-37-generic
**Build:** Debug build from source (`./build/bin/`)

---

## Executive Summary

✅ **UAT Environment is READY for testing**

The YAMY Linux implementation is running and functional. The key remapping engine is operational with configuration loaded and IPC commands responding correctly.

---

## Current Status

### System Status
- **Process ID:** 766169
- **Memory Usage:** 34.9 MB
- **CPU Usage:** 0.0% (idle)
- **Uptime:** Running stable

### Engine Status
```
Engine:    Running
Config:    master.mayu
Keymap:    Global
Keys:      0 (waiting for input)
```

### Configuration Details
- **Name:** master.mayu
- **Path:** /home/rmondo/repos/yamy/keymaps/master.mayu
- **Loaded:** 2025-12-14T10:24:53
- **Keymaps:** Global
- **Substitutions:** 82 key mappings

### Input Devices Hooked
The engine has successfully hooked the following keyboard devices:
1. `/dev/input/event7` - Kingston HyperX SoloCast Consumer Control
2. `/dev/input/event3` - USB Keyboard
3. `/dev/input/event5` - USB Keyboard Consumer Control

### IPC Control Server
- **Socket:** /tmp/yamy-engine.sock
- **Status:** Active and responding
- **Commands Tested:** ✅ status, config, keymaps, metrics, start, stop

---

## Components Verified

### ✅ Working
1. **Binary Execution** - Yamy starts successfully
2. **IPC Server** - Unix socket communication working
3. **Configuration Loading** - Session restore loads master.mayu
4. **Engine Start/Stop** - Engine can be started and stopped via IPC
5. **Input Device Hooking** - Successfully grabbed 3 keyboard devices
6. **Virtual Input Device** - Created at /dev/input/event23
7. **Qt GUI** - Application initialized (system tray should be visible)
8. **Performance Metrics** - Monitoring system operational
9. **Global Hotkey** - Ctrl+Alt+C registered

### ⚠️ Known Issue (Critical)
**Configuration Reload While Engine Running**
- **Status:** CRASH - SIGABRT
- **Details:** See UAT_ISSUES.md
- **Impact:** Cannot reload configuration while engine is running
- **Workaround:** Stop engine before reload, or restart yamy

---

## UAT Test Readiness

### Ready for Testing
1. **Basic Key Remapping** ✅
   - Configuration loaded with 82 substitutions
   - Engine is running and monitoring keyboards
   - Ready to test actual key remapping behavior

2. **IPC Commands** ✅
   - All commands functional: status, config, keymaps, metrics, start, stop
   - yamy-ctl CLI tool working correctly

3. **Session Persistence** ✅
   - Session file: ~/.config/yamy/session.json
   - Configuration auto-restored on startup

4. **GUI Components** ✅
   - Qt GUI initialized
   - System tray icon should be visible
   - Global hotkey (Ctrl+Alt+C) registered

### Test Scenarios Available

#### Test 1: Basic Key Remapping (READY)
The current configuration (config_clean.mayu) includes Dvorak-like substitutions:
- A → Tab
- B → Enter
- C → Delete
- M/V → Backspace
- Number keys remapped (e.g., 1 → LShift)

**To test:** Open any text editor and press these keys to verify remapping works.

#### Test 2: System Tray Interaction (READY)
- Check for yamy icon in system tray
- Right-click for menu (Settings, About, Quit)
- Test opening Settings dialog

#### Test 3: Engine Control (READY)
```bash
./build/bin/yamy-ctl stop    # Stop engine
./build/bin/yamy-ctl start   # Start engine
./build/bin/yamy-ctl status  # Check status
```

#### Test 4: Performance Monitoring (READY)
```bash
# Type some keys, then check metrics
./build/bin/yamy-ctl metrics
# Should show latency and keys/second statistics
```

---

## Test Configuration Files

### Production Config
- **Location:** /home/rmondo/repos/yamy/keymaps/master.mayu
- **Status:** Currently loaded
- **Mappings:** 82 substitutions

### UAT Test Config (Alternative)
- **Location:** /tmp/uat-test.mayu
- **Purpose:** Minimal test configuration for basic testing
- **Mappings:** CapsLock → Escape, Ctrl+H → Backspace
- **Note:** ⚠️ Cannot reload while engine running due to crash bug

---

## Commands for UAT Testing

### Check Status
```bash
./build/bin/yamy-ctl status
./build/bin/yamy-ctl config
./build/bin/yamy-ctl keymaps
./build/bin/yamy-ctl metrics
```

### Control Engine
```bash
./build/bin/yamy-ctl stop
./build/bin/yamy-ctl start
```

### Monitor Logs
```bash
# Check for new output from yamy
tail -f /tmp/claude/tasks/b915dfb.output
```

### Check Crash Reports (if any)
```bash
ls -lt ~/.local/share/yamy/crashes/
```

---

## What to Test (UAT Checklist)

### Critical Functionality
- [ ] **Keyboard remapping works** - Press remapped keys and verify they produce correct output
- [ ] **System tray icon visible** - Check top panel or hidden icons
- [ ] **GUI responds to clicks** - Right-click tray icon and test menu
- [ ] **Engine can be stopped/started** - Test via yamy-ctl or GUI
- [ ] **Metrics update** - Check if latency and key counts increase with typing

### Advanced Testing
- [ ] **Session persistence** - Restart yamy and verify config auto-loads
- [ ] **Multiple keymaps** - Test switching between keymaps (if configured)
- [ ] **Global hotkey** - Press Ctrl+Alt+C to test hotkey registration
- [ ] **Performance under load** - Type rapidly and check metrics

### Known Limitations
- ❌ **Cannot reload config while engine running** - Use workaround
- ⚠️ **Config reload causes crash** - Documented in UAT_ISSUES.md

---

## Support Files

- **Issue Tracker:** UAT_ISSUES.md
- **Manual Test Guide:** MANUAL-TEST-GUIDE.md
- **Session File:** ~/.config/yamy/session.json
- **Crash Reports:** ~/.local/share/yamy/crashes/

---

## Ready for UAT

**Status:** ✅ **READY**

The YAMY Linux implementation is running and ready for User Acceptance Testing. The engine is operational, configuration is loaded, and all IPC commands are responding correctly.

**Start Testing:** Begin with Test 1 (Basic Key Remapping) by opening a text editor and pressing the remapped keys defined in config_clean.mayu.

**Need Help?** Refer to MANUAL-TEST-GUIDE.md for detailed test procedures.

---

**Report Generated:** 2025-12-14 10:26
**Prepared by:** Claude Code UAT Setup
