
from dataclasses import dataclass, asdict
from typing import Optional

@dataclass
class KeyMapping:
    input_key: str
    output_key: str
    input_evdev: int
    output_evdev: int

@dataclass
class TestResult:
    mapping: KeyMapping
    event_type: str
    passed: bool
    expected_evdev: int
    actual_evdev: Optional[int]
    error_message: Optional[str] = None
