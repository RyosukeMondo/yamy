
import subprocess
import os

class EventInjector:
    def __init__(self, yamy_test_path: str = './build/bin/yamy-test'):
        self.yamy_test_path = yamy_test_path
        self.proc = None

    def start(self):
        """Starts the yamy-test interactive process."""
        if self.proc:
            print("[EventInjector] yamy-test is already running.")
            return

        print("[EventInjector] Starting yamy-test interactive...")
        try:
            self.proc = subprocess.Popen(
                [self.yamy_test_path, 'interactive'],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                text=True,
                bufsize=1,
                # Create a new process group so we can kill it and its children
                preexec_fn=os.setsid 
            )

            # Wait for the "READY" signal
            while True:
                line = self.proc.stdout.readline().strip()
                if not line:
                    raise RuntimeError("Failed to start yamy-test interactive (EOF).")
                if line == "READY":
                    break
                print(f"[EventInjector] yamy-test: {line}")
            
            print("[EventInjector] Virtual keyboard created and ready.")
        except FileNotFoundError:
            print(f"[EventInjector] ERROR: yamy-test executable not found at '{self.yamy_test_path}'")
            self.proc = None
            raise

    def stop(self):
        """Stops the yamy-test interactive process."""
        if not self.proc:
            return

        print("[EventInjector] Stopping yamy-test interactive...")
        try:
            # Send EXIT command and close stdin
            if self.proc.stdin:
                self.proc.stdin.write("EXIT\n")
                self.proc.stdin.close()

            # Terminate the process group
            os.killpg(os.getpgid(self.proc.pid), 15) # SIGTERM
            self.proc.wait(timeout=5)
        except (ProcessLookupError, BrokenPipeError):
            # Process might already be gone
            pass
        except subprocess.TimeoutExpired:
            print("[EventInjector] yamy-test did not terminate gracefully, sending SIGKILL.")
            os.killpg(os.getpgid(self.proc.pid), 9) # SIGKILL
        finally:
            self.proc = None
            print("[EventInjector] Stopped.")

    def send_command(self, cmd: str):
        """Sends a command to the yamy-test process."""
        if not self.proc or not self.proc.stdin:
            raise RuntimeError("EventInjector is not running.")

        try:
            self.proc.stdin.write(cmd + "\n")
            # Wait for "OK" confirmation
            response = self.proc.stdout.readline().strip()
            if response != "OK":
                raise RuntimeError(f"yamy-test returned an unexpected response: {response}")
        except BrokenPipeError:
            raise RuntimeError("Connection to yamy-test process lost.")

    def press(self, evdev_code: int):
        self.send_command(f"PRESS {evdev_code}")

    def release(self, evdev_code: int):
        self.send_command(f"RELEASE {evdev_code}")

    def tap(self, evdev_code: int):
        self.press(evdev_code)
        self.release(evdev_code)
        
    def wait(self, ms: int):
        self.send_command(f"WAIT {ms}")

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stop()

