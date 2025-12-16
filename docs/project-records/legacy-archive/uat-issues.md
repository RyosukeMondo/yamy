# UAT Test Issues - Linux Implementation

## Critical Bug: Crash on Configuration Reload

**Date:** 2025-12-14
**Status:** CRITICAL - Blocks UAT testing

### Description
YAMY crashes with a fatal glibc assertion failure when attempting to reload configuration via IPC while the engine is running.

### Steps to Reproduce
1. Start yamy: `./build/bin/yamy --no-restore &`
2. Start the engine: `./build/bin/yamy-ctl start`
3. Attempt to reload configuration: `./build/bin/yamy-ctl reload --config /path/to/config.mayu`

### Error Message
```
Fatal glibc error: tpp.c:83 (__pthread_tpp_change_priority): assertion failed:
new_prio == -1 || (new_prio >= fifo_min_prio && new_prio <= fifo_max_prio)
```

### Crash Details
- **Signal:** SIGABRT (6)
- **Crash Report:** `/home/rmondo/.local/share/yamy/crashes/crash_1765675442.txt`
- **Context:** Occurs during `Engine::stop()` when handling reload command

### Impact
- **Severity:** HIGH
- Users cannot reload configurations while engine is running
- Workaround: Stop engine before reload, or restart yamy

### Root Cause Analysis
The crash occurs in pthread priority management code within glibc. This suggests:
1. The engine's thread priority is being set to an invalid value
2. Thread cleanup during engine stop is attempting to restore an invalid priority
3. Possible issue with real-time scheduling policy

### Recommended Fix
1. Review thread priority settings in `Engine::start()` and `Engine::stop()`
2. Ensure priority values are within valid FIFO scheduler range
3. Add proper error handling for thread priority operations
4. Consider using SCHED_OTHER instead of SCHED_FIFO if RT scheduling is not critical

### Workaround for UAT Testing
1. Load configuration before starting the engine
2. OR restart yamy completely to load a new configuration
3. Use GUI to load configuration (may have better error handling)

### Test Environment
- **OS:** Linux 6.14.0-37-generic
- **Distribution:** Ubuntu/Debian
- **Build:** Debug build from source
- **User Groups:** input (verified)
