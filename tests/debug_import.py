import sys
import os
print("Starting debug...")
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
print(f"Path: {sys.path}")
try:
    from framework.parser import MayuParser
    print("Imported parser")
except Exception as e:
    print(f"Failed parser: {e}")

try:
    import evdev
    print("Imported evdev")
except Exception as e:
    print(f"Failed evdev: {e}")
