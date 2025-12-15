#!/usr/bin/env python3
"""
Creates a persistent virtual keyboard for YAMY testing.
YAMY will grab this device at startup, then we can inject events to it.
"""
import sys
import time
import os
from evdev import UInput, ecodes as e

def main():
    device_name = sys.argv[1] if len(sys.argv) > 1 else "YAMY-Test-Keyboard"

    # Create virtual keyboard with all key capabilities
    cap = {
        e.EV_KEY: list(range(e.KEY_ESC, e.KEY_MICMUTE + 1))
    }

    ui = UInput(cap, name=device_name)
    print(f"Created test keyboard: {device_name}", file=sys.stderr)
    if hasattr(ui, 'device') and ui.device:
        print(f"Device: {ui.device.path}", file=sys.stderr)
    print("READY", flush=True)  # Signal that device is created

    # Keep device alive
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        ui.close()
        print("Test keyboard closed", file=sys.stderr)

if __name__ == "__main__":
    main()
