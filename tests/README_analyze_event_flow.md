# Event Flow Analysis Tool

## Overview

`analyze_event_flow.py` is a Python script that parses YAMY debug logs to analyze keyboard event flow through the three processing layers:

1. **Layer 1**: evdev → YAMY scan code conversion
2. **Layer 2**: Substitution application (or passthrough)
3. **Layer 3**: YAMY scan code → evdev output conversion

## Usage

```bash
# Basic usage - text output to stdout
python3 tests/analyze_event_flow.py /path/to/debug.log

# JSON output
python3 tests/analyze_event_flow.py /path/to/debug.log --format json

# Save to file
python3 tests/analyze_event_flow.py /path/to/debug.log -o report.txt
python3 tests/analyze_event_flow.py /path/to/debug.log --format json -o report.json
```

## Features

### Statistics
- Total events processed
- Complete flows (all 3 layers present)
- Incomplete flows
- Missing layer counts
- PRESS vs RELEASE event counts
- Completion rate percentage

### Missing Layer Detection
Identifies events that don't complete the full layer sequence:
- Events that reach Layer 1 but not Layer 2
- Events that reach Layer 2 but not Layer 3
- Events that skip multiple layers

### Asymmetric Event Detection
Detects keys that behave differently for PRESS vs RELEASE events:
- Keys that only work on RELEASE (common bug pattern)
- Keys that only work on PRESS
- Completion rate comparison between event types

### Complete Flow Examples
Shows sample events that successfully complete all 3 layers for validation.

## Log Format Requirements

The script expects YAMY debug logs with the following format:

```
[LAYER1:IN] evdev <num> (<KEY_NAME>) <PRESS|RELEASE> → yamy 0x<hex>
[LAYER1:IN] evdev <num> (<KEY_NAME>) <PRESS|RELEASE> → NOT FOUND
[LAYER2:IN] Processing yamy 0x<hex>
[LAYER2:SUBST] 0x<hex> -> 0x<hex>
[LAYER2:PASSTHROUGH] 0x<hex> (no substitution)
[LAYER3:OUT] yamy 0x<hex> → evdev <num> (<KEY_NAME>) - Found in <map_name>
[LAYER3:OUT] yamy 0x<hex> → NOT FOUND in any map
```

Enable debug logging in YAMY by setting:
```bash
export YAMY_DEBUG_KEYCODE=1
```

## Example Output

```
================================================================================
YAMY Event Flow Analysis Report
================================================================================

STATISTICS
--------------------------------------------------------------------------------
Total log lines parsed: 1234
Total events found: 87
Complete flows (all 3 layers): 65
Incomplete flows: 22
Completion rate: 74.7%
PRESS events: 44
RELEASE events: 43

MISSING LAYERS
--------------------------------------------------------------------------------
Events missing Layer 2: 10
Events missing Layer 3: 5
Events missing both Layer 2 and 3: 7

ASYMMETRIC EVENTS (different behavior for PRESS vs RELEASE)
--------------------------------------------------------------------------------
KEY_R: PRESS=0%, RELEASE=100%
KEY_T: PRESS=0%, RELEASE=100%
KEY_N: PRESS=0%, RELEASE=0%
```

## Dependencies

- Python 3.6 or higher
- Standard library only (no external dependencies)

## Use Cases

1. **Debugging key remapping issues**: Identify which layer is causing a problem
2. **Validation testing**: Verify all substitutions work for both PRESS and RELEASE
3. **Regression detection**: Compare completion rates before/after code changes
4. **Performance analysis**: Track event flow counts and patterns
5. **CI/CD integration**: Generate machine-readable JSON reports for automated testing

## JSON Output Schema

The JSON format includes:
```json
{
  "statistics": {
    "total_events": <number>,
    "complete_flows": <number>,
    "incomplete_flows": <number>,
    "completion_rate": "<percentage>"
  },
  "missing_layers": {
    "missing_layer2_count": <number>,
    "missing_layer3_count": <number>,
    "samples": [...]
  },
  "asymmetric_events": {
    "<key_name>": {
      "press_completion": "<percentage>",
      "release_completion": "<percentage>",
      "samples": {...}
    }
  }
}
```

## Testing

A sample test log is provided in `tests/test_sample.log`:

```bash
python3 tests/analyze_event_flow.py tests/test_sample.log
```

This demonstrates the tool's ability to detect complete flows, missing layers, and asymmetric events.
