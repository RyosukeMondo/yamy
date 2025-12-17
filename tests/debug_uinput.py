
import evdev
from evdev import UInput, ecodes as e
import time
import select

def main():
    print("Creating UInput device...")
    cap = {
        e.EV_KEY: [e.KEY_A, e.KEY_B]
    }
    
    try:
        ui = UInput(cap, name='test-uinput-device')
        print(f"UInput wrapper created.")
        if ui.device:
            print(f"UInput device detected at {ui.device.path}")
            device_path = ui.device.path
        else:
            print("ui.device is None. Searching manually...")
            device_path = None
            import os
            # Give udev some time
            time.sleep(2)
            for path in evdev.list_devices():
                d = evdev.InputDevice(path)
                if d.name == 'test-uinput-device':
                    print(f"Found manually at {path}")
                    device_path = path
                    break
            
            if not device_path:
                print("Could not find device manually.")
                return

    except Exception as err:
        print(f"Failed to create UInput device: {err}")
        return

    # Wait for the device to be ready/detected
    time.sleep(1)

    print("Opening the device for reading...")
    try:
        dev = evdev.InputDevice(device_path)
        print(f"Opened device: {dev.name} at {dev.path}")
    except Exception as err:
        print(f"Failed to open device for reading: {err}")
        ui.close()
        return

    print("Injecting KEY_A PRESS...")
    ui.write(e.EV_KEY, e.KEY_A, 1)
    ui.syn()

    print("Waiting for event...")
    r, w, x = select.select([dev.fd], [], [], 2.0)
    
    if r:
        print("Data available on fd!")
        for event in dev.read():
            print(f"Received event: {event}")
    else:
        print("Timeout! No event received.")

    print("Injecting KEY_A RELEASE...")
    ui.write(e.EV_KEY, e.KEY_A, 0)
    ui.syn()
    
    time.sleep(0.1)
    ui.close()
    dev.close()

if __name__ == "__main__":
    main()
