
import time
from select import select
from typing import Optional, Tuple

try:
    import evdev
    from evdev import ecodes, InputDevice, list_devices
except ImportError:
    print("ERROR: python3-evdev is required for output capture.")
    evdev = None

class LogMonitor:
    def __init__(self, device_name: str = "Yamy Remapped Output Device"):
        self.device_name = device_name
        self.device: Optional[InputDevice] = None # Use self.device consistently

    def find_device(self) -> bool:
        """Finds the virtual device created by YAMY."""
        if not evdev:
            return False
            
        print(f"[LogMonitor] Searching for output device: '{self.device_name}'...")
        found = False # Initialize found here
        for path in list_devices():
            try:
                d = InputDevice(path)
                if self.device_name in d.name: # Use self.device_name for consistency
                    self.device = d # Use self.device consistently
                    print(f"[LogMonitor] Found output device: {d.name} at {path}")
                    found = True
                    break # Break once found
            except Exception as e:
                # Can fail on some devices, e.g. permission denied
                print(f"[LogMonitor] Warning: Could not inspect device at {path}: {e}")
        
        if not found: # Only print error if not found after loop
            print(f"[LogMonitor] ERROR: Could not find output device named '{self.device_name}'.")
        return found # Return found status

    def start(self):
        """Starts monitoring by finding and grabbing the device."""
        if self.device:
            print("[LogMonitor] Already started.")
            return

        if not self.find_device():
            raise RuntimeError("LogMonitor could not find the required device.")

        try:
            # Grab the device to prevent other applications from receiving events
            # self.device.grab()
            # print("[LogMonitor] Device grabbed exclusively.")
            print("[LogMonitor] Device NOT grabbed (debug).")
        except Exception as e:
            print(f"[LogMonitor] Warning: Could not grab device. Events may leak. Error: {e}")

    def stop(self):
        """Stops monitoring and releases the device."""
        if not self.device:
            return

        print("[LogMonitor] Stopping monitor...")
        try:
            # self.device.ungrab()
            self.device.close()
            print("[LogMonitor] Device released.")
        except Exception as e:
            print(f"[LogMonitor] Warning: Error while releasing device: {e}")
        finally:
            self.device = None
    
    def drain_events(self):
        """Reads and discards all pending events."""
        if not self.device: return
        
        while True:
            r, _, _ = select([self.device.fd], [], [], 0)
            if r:
                try:
                    # Read all events in one go
                    for _ in self.device.read():
                        pass
                except (IOError, BlockingIOError):
                    # No more events to read
                    break
            else:
                # No more events pending
                break

    def capture_next_key_event(self, timeout: float = 2.0) -> Optional[Tuple[int, int]]:
        """
        Captures the next single EV_KEY event from the device.

        Args:
            timeout: Time to wait for an event in seconds.

        Returns:
            A tuple of (evdev_code, value) or None if a timeout occurs.
            value 1 = PRESS, 0 = RELEASE, 2 = REPEAT
        """
        if not self.device:
            raise RuntimeError("LogMonitor is not running.")

        start_time = time.time()
        while time.time() - start_time < timeout:
            # Use select for non-blocking check with a short timeout
            r, _, _ = select([self.device.fd], [], [], 0.1)
            if r:
                try:
                    for event in self.device.read():
                        print(f"[LogMonitor Debug] Event: type={event.type}, code={event.code}, value={event.value}") # DEBUG
                        if event.type == ecodes.EV_KEY:
                            return (event.code, event.value)
                except (IOError, BlockingIOError):
                    # This can happen if there are no events to read, so we just continue
                    pass
        
        return None # Timeout

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stop()
