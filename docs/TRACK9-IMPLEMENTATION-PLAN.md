# Track 9: evdev Input Capture - Implementation Plan

## Architecture Analysis

### Windows Implementation (Reference)
- **Hook Method:** Low-level keyboard/mouse hooks (SetWindowsHookEx)
- **Threading:** Separate thread per hook with message loop
- **Event Flow:** Windows → Hook → Detour → Callback → Suppress/Pass
- **Injection Filter:** LLKHF_INJECTED flag prevents feedback loops
- **Data Structure:** KBDLLHOOKSTRUCT → KeyEvent conversion

### Linux evdev Equivalent
- **Hook Method:** Direct evdev device access (/dev/input/eventX)
- **Threading:** One thread per keyboard device + hotplug monitor
- **Event Flow:** evdev → Read thread → Convert → Callback → Suppress/Pass
- **Injection Filter:** Track our own injected events by timestamp/extraInfo
- **Data Structure:** struct input_event → KeyEvent conversion

## Implementation Phases

### Phase 1: Setup Script & Permissions ✅
**Files:** `linux_setup.sh`, `docs/LINUX-SETUP.md`
**Dependencies:** None
**Time:** 30 minutes

Tasks:
- [x] Create setup script for permissions
- [x] Add user to input group
- [x] Verify /dev/input access
- [x] Check udev rules
- [x] Documentation

### Phase 2: Device Enumeration (libudev) 🔨
**Files:** `device_manager_linux.h`, `device_manager_linux.cpp`
**Dependencies:** libudev
**Time:** 2 hours

Tasks:
- [ ] udev context creation
- [ ] Enumerate input devices
- [ ] Filter keyboard devices (capabilities check)
- [ ] Get device path and properties
- [ ] Handle device enumeration errors

Key APIs:
```cpp
udev* udev_new()
udev_enumerate* udev_enumerate_new()
udev_enumerate_add_match_subsystem()
udev_enumerate_scan_devices()
udev_list_entry_get_name()
```

### Phase 3: Device Manager Class 🔨
**Files:** `device_manager_linux.h`, `device_manager_linux.cpp`
**Dependencies:** Phase 2, pthread
**Time:** 2 hours

Tasks:
- [ ] DeviceManager class structure
- [ ] Open device files (open /dev/input/eventX)
- [ ] Test device capabilities (EVIOCGBIT)
- [ ] Grab exclusive access (EVIOCGRAB)
- [ ] Release devices on shutdown
- [ ] Error handling for permissions

Key APIs:
```cpp
open("/dev/input/eventX", O_RDONLY | O_NONBLOCK)
ioctl(fd, EVIOCGRAB, 1)  // Exclusive grab
ioctl(fd, EVIOCGBIT(EV_KEY, size), bits)  // Check capabilities
```

### Phase 4: Event Reader Thread 🔨
**Files:** `input_hook_linux.cpp` (EventReader class)
**Dependencies:** Phase 3, keycode_mapping
**Time:** 2 hours

Tasks:
- [ ] Thread per device
- [ ] Read loop (read struct input_event)
- [ ] Convert evdev codes to YAMY codes (use Track 11)
- [ ] Fill KeyEvent structure
- [ ] Call callback
- [ ] Handle thread lifecycle
- [ ] Graceful shutdown

Key APIs:
```cpp
pthread_create()
read(fd, &event, sizeof(struct input_event))
// event.type = EV_KEY
// event.code = KEY_A
// event.value = 0 (release) or 1 (press) or 2 (repeat)
```

### Phase 5: InputHookLinux Main Class 🔨
**Files:** `input_hook_linux.cpp`, `input_hook_linux.h`
**Dependencies:** Phase 4
**Time:** 1.5 hours

Tasks:
- [ ] Replace stub implementation
- [ ] install() - enumerate devices, grab, start threads
- [ ] uninstall() - stop threads, ungrab, close devices
- [ ] isInstalled() - track state
- [ ] Store callbacks (KeyCallback, MouseCallback)
- [ ] Injection filter (prevent feedback loops)
- [ ] Thread safety (mutexes)

### Phase 6: Hotplug Support (Optional) ⏳
**Files:** `device_manager_linux.cpp` (hotplug monitor)
**Dependencies:** Phase 5
**Time:** 1 hour (deferred)

Tasks:
- [ ] udev monitor for device add/remove
- [ ] Detect new keyboards
- [ ] Auto-grab new devices
- [ ] Handle device disconnection

## Data Flow Diagram

```
┌─────────────────┐
│  /dev/input/    │
│  event0..N      │
└────────┬────────┘
         │
         │ read() struct input_event
         │
┌────────▼────────┐
│  EventReader    │
│  Thread         │
│  (per device)   │
└────────┬────────┘
         │
         │ evdev keycode
         │
┌────────▼────────────┐
│  evdevToYamyKeyCode │
│  (Track 11)         │
└────────┬────────────┘
         │
         │ KeyEvent
         │
┌────────▼────────┐
│  KeyCallback    │
│  (from Engine)  │
└────────┬────────┘
         │
         │ return true (suppress) or false (pass)
         │
┌────────▼────────┐
│  uinput inject  │
│  (Track 10)     │
│  OR discard     │
└─────────────────┘
```

## Implementation Strategy

### Step-by-Step Order

1. **Setup first** (Phase 1) - Get permissions working
2. **Device enumeration** (Phase 2) - Find keyboards
3. **Device manager** (Phase 3) - Open and grab devices
4. **Event reader** (Phase 4) - Read and convert events
5. **Main class** (Phase 5) - Integrate everything
6. **Test incrementally** - After each phase

### Testing Approach

After each phase:
```bash
# Phase 2 test
./test_enumerate_devices

# Phase 3 test
./test_device_grab

# Phase 4 test
./test_event_reader  # Should print key events

# Phase 5 test
./test_input_hook    # Full integration
```

## Error Handling

### Permission Errors
- Check user in input group: `groups $USER | grep input`
- Check file permissions: `ls -la /dev/input/event*`
- Provide clear error messages

### Device Access Errors
- Handle EACCES (permission denied)
- Handle EBUSY (device already grabbed)
- Handle device disconnection gracefully

### Thread Safety
- Use mutexes for callback access
- Use atomic bool for shutdown flag
- Join threads properly in destructor

## Security Considerations

1. **Exclusive Grab:** EVIOCGRAB prevents other apps from reading
2. **Injection Loop:** Filter events we injected (timestamp tracking)
3. **Root Not Required:** Input group membership sufficient
4. **Proper Cleanup:** Always ungrab devices on shutdown

## Dependencies Check

```bash
# Required packages
sudo apt install libudev-dev linux-headers-$(uname -r)

# Verify
ls /dev/input/event*
getfacl /dev/input/event0
```

## Success Metrics

- [ ] Can enumerate all keyboard devices
- [ ] Can grab devices without root
- [ ] Can read key events
- [ ] Correctly converts evdev → YAMY codes
- [ ] Callback receives events
- [ ] Cleanup works properly
- [ ] No memory leaks
- [ ] Thread-safe operation

## Timeline

- Phase 1: 30 min
- Phase 2: 2 hours
- Phase 3: 2 hours
- Phase 4: 2 hours
- Phase 5: 1.5 hours
- **Total: 8 hours** (matches original estimate)

## Next Steps

1. Create `linux_setup.sh`
2. Create `LINUX-SETUP.md` documentation
3. Implement Phase 2 (device enumeration)
4. Continue through phases sequentially
