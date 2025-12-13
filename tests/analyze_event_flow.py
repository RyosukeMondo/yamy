#!/usr/bin/env python3
"""
Log Analysis Script for YAMY Key Event Flow

Parses debug logs to extract event sequences, identify missing layers,
and generate comprehensive reports showing key → LAYER1 → LAYER2 → LAYER3 progression.
"""

import re
import sys
import json
from collections import defaultdict
from dataclasses import dataclass, asdict
from typing import Dict, List, Optional, Set
from enum import Enum


class EventType(Enum):
    PRESS = "PRESS"
    RELEASE = "RELEASE"


@dataclass
class Layer1Event:
    evdev: int
    key_name: str
    event_type: EventType
    yamy_code: Optional[int]
    found: bool
    timestamp: str


@dataclass
class Layer2Event:
    input_yamy: int
    output_yamy: Optional[int]
    is_substitution: bool
    timestamp: str


@dataclass
class Layer3Event:
    yamy_code: int
    evdev: Optional[int]
    key_name: Optional[str]
    map_source: Optional[str]
    found: bool
    timestamp: str


@dataclass
class EventFlow:
    """Complete event flow through all layers"""
    layer1: Optional[Layer1Event] = None
    layer2: Optional[Layer2Event] = None
    layer3: Optional[Layer3Event] = None

    def has_missing_layers(self) -> bool:
        """Check if any layers are missing in the flow"""
        return (self.layer1 and not self.layer2) or \
               (self.layer2 and not self.layer3) or \
               (self.layer1 and not self.layer3)

    def is_complete(self) -> bool:
        """Check if all layers are present"""
        return self.layer1 is not None and self.layer2 is not None and self.layer3 is not None

    def is_asymmetric(self) -> bool:
        """Check for asymmetric behavior (e.g., RELEASE-only, layer skipping)"""
        if not self.layer1:
            return False
        # If we have layer1 but not all subsequent layers, it's asymmetric
        return not self.is_complete()


class LogParser:
    """Parser for YAMY debug logs"""

    # Regex patterns for log parsing
    LAYER1_PATTERN = re.compile(
        r'\[LAYER1:IN\] evdev (\d+) \(([^)]+)\) (PRESS|RELEASE) → (.+)$'
    )
    LAYER1_NOT_FOUND_PATTERN = re.compile(
        r'\[LAYER1:IN\] evdev (\d+) \(([^)]+)\) (PRESS|RELEASE) → NOT FOUND$'
    )

    LAYER2_IN_PATTERN = re.compile(
        r'\[LAYER2:IN\] Processing yamy 0x([0-9A-Fa-f]+)'
    )
    LAYER2_SUBST_PATTERN = re.compile(
        r'\[LAYER2:SUBST\] 0x([0-9A-Fa-f]+) -> 0x([0-9A-Fa-f]+)'
    )
    LAYER2_PASSTHROUGH_PATTERN = re.compile(
        r'\[LAYER2:PASSTHROUGH\] 0x([0-9A-Fa-f]+)'
    )

    LAYER3_PATTERN = re.compile(
        r'\[LAYER3:OUT\] yamy 0x([0-9A-Fa-f]+) → evdev (\d+) \(([^)]+)\) - Found in (.+)$'
    )
    LAYER3_NOT_FOUND_PATTERN = re.compile(
        r'\[LAYER3:OUT\] yamy 0x([0-9A-Fa-f]+) → NOT FOUND in any map$'
    )

    TIMESTAMP_PATTERN = re.compile(r'^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})')

    def __init__(self):
        self.events: List[Dict] = []
        self.flows: List[EventFlow] = []
        self.current_flow: Optional[EventFlow] = None
        self.line_count = 0

    def parse_timestamp(self, line: str) -> str:
        """Extract timestamp from log line"""
        match = self.TIMESTAMP_PATTERN.search(line)
        return match.group(1) if match else ""

    def parse_layer1(self, line: str, timestamp: str) -> Optional[Layer1Event]:
        """Parse Layer 1 log entries"""
        # Try NOT FOUND pattern first
        match = self.LAYER1_NOT_FOUND_PATTERN.search(line)
        if match:
            evdev = int(match.group(1))
            key_name = match.group(2)
            event_type = EventType[match.group(3)]
            return Layer1Event(
                evdev=evdev,
                key_name=key_name,
                event_type=event_type,
                yamy_code=None,
                found=False,
                timestamp=timestamp
            )

        # Try normal pattern
        match = self.LAYER1_PATTERN.search(line)
        if match:
            evdev = int(match.group(1))
            key_name = match.group(2)
            event_type = EventType[match.group(3)]
            yamy_str = match.group(4)

            # Extract yamy code
            yamy_match = re.search(r'yamy 0x([0-9A-Fa-f]+)', yamy_str)
            yamy_code = int(yamy_match.group(1), 16) if yamy_match else None

            return Layer1Event(
                evdev=evdev,
                key_name=key_name,
                event_type=event_type,
                yamy_code=yamy_code,
                found=yamy_code is not None,
                timestamp=timestamp
            )

        return None

    def parse_layer2(self, line: str, timestamp: str) -> Optional[Layer2Event]:
        """Parse Layer 2 log entries"""
        # Check for LAYER2:IN (processing start)
        match = self.LAYER2_IN_PATTERN.search(line)
        if match:
            # This marks the start of Layer 2, but we need SUBST or PASSTHROUGH for complete info
            return None

        # Check for LAYER2:SUBST
        match = self.LAYER2_SUBST_PATTERN.search(line)
        if match:
            input_yamy = int(match.group(1), 16)
            output_yamy = int(match.group(2), 16)
            return Layer2Event(
                input_yamy=input_yamy,
                output_yamy=output_yamy,
                is_substitution=True,
                timestamp=timestamp
            )

        # Check for LAYER2:PASSTHROUGH
        match = self.LAYER2_PASSTHROUGH_PATTERN.search(line)
        if match:
            input_yamy = int(match.group(1), 16)
            return Layer2Event(
                input_yamy=input_yamy,
                output_yamy=input_yamy,
                is_substitution=False,
                timestamp=timestamp
            )

        return None

    def parse_layer3(self, line: str, timestamp: str) -> Optional[Layer3Event]:
        """Parse Layer 3 log entries"""
        # Try NOT FOUND pattern first
        match = self.LAYER3_NOT_FOUND_PATTERN.search(line)
        if match:
            yamy_code = int(match.group(1), 16)
            return Layer3Event(
                yamy_code=yamy_code,
                evdev=None,
                key_name=None,
                map_source=None,
                found=False,
                timestamp=timestamp
            )

        # Try normal pattern
        match = self.LAYER3_PATTERN.search(line)
        if match:
            yamy_code = int(match.group(1), 16)
            evdev = int(match.group(2))
            key_name = match.group(3)
            map_source = match.group(4)
            return Layer3Event(
                yamy_code=yamy_code,
                evdev=evdev,
                key_name=key_name,
                map_source=map_source,
                found=True,
                timestamp=timestamp
            )

        return None

    def parse_file(self, filepath: str):
        """Parse log file and extract event flows"""
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            current_flow = EventFlow()

            for line in f:
                self.line_count += 1
                line = line.strip()
                if not line:
                    continue

                timestamp = self.parse_timestamp(line)

                # Try to parse each layer
                layer1 = self.parse_layer1(line, timestamp)
                if layer1:
                    # New event starts - save previous flow if it has data
                    if current_flow.layer1 or current_flow.layer2 or current_flow.layer3:
                        self.flows.append(current_flow)
                    current_flow = EventFlow(layer1=layer1)
                    continue

                layer2 = self.parse_layer2(line, timestamp)
                if layer2:
                    current_flow.layer2 = layer2
                    continue

                layer3 = self.parse_layer3(line, timestamp)
                if layer3:
                    current_flow.layer3 = layer3
                    # Flow complete, save it
                    if current_flow.layer1:
                        self.flows.append(current_flow)
                        current_flow = EventFlow()

            # Save last flow if any
            if current_flow.layer1 or current_flow.layer2 or current_flow.layer3:
                self.flows.append(current_flow)


class FlowAnalyzer:
    """Analyzes parsed event flows for issues and patterns"""

    def __init__(self, flows: List[EventFlow]):
        self.flows = flows

    def get_statistics(self) -> Dict:
        """Calculate overall statistics"""
        total = len(self.flows)
        complete = sum(1 for f in self.flows if f.is_complete())
        missing_layer2 = sum(1 for f in self.flows if f.layer1 and not f.layer2)
        missing_layer3 = sum(1 for f in self.flows if f.layer1 and not f.layer3)
        asymmetric = sum(1 for f in self.flows if f.is_asymmetric())

        # Count by event type
        press_events = sum(1 for f in self.flows if f.layer1 and f.layer1.event_type == EventType.PRESS)
        release_events = sum(1 for f in self.flows if f.layer1 and f.layer1.event_type == EventType.RELEASE)

        return {
            "total_events": total,
            "complete_flows": complete,
            "incomplete_flows": total - complete,
            "missing_layer2": missing_layer2,
            "missing_layer3": missing_layer3,
            "asymmetric": asymmetric,
            "press_events": press_events,
            "release_events": release_events,
            "completion_rate": f"{(complete/total*100):.1f}%" if total > 0 else "0%"
        }

    def get_missing_layer_events(self) -> Dict[str, List[EventFlow]]:
        """Get events that are missing specific layers"""
        return {
            "missing_layer2": [f for f in self.flows if f.layer1 and not f.layer2],
            "missing_layer3": [f for f in self.flows if f.layer2 and not f.layer3],
            "missing_both": [f for f in self.flows if f.layer1 and not f.layer2 and not f.layer3]
        }

    def get_asymmetric_events(self) -> Dict[str, List[EventFlow]]:
        """Get events with asymmetric behavior"""
        by_key: Dict[str, Dict[str, List[EventFlow]]] = defaultdict(lambda: {"PRESS": [], "RELEASE": []})

        for flow in self.flows:
            if flow.layer1:
                key_name = flow.layer1.key_name
                event_type = flow.layer1.event_type.value
                by_key[key_name][event_type].append(flow)

        # Find keys that work differently for PRESS vs RELEASE
        asymmetric = {}
        for key_name, events in by_key.items():
            press_complete = sum(1 for f in events["PRESS"] if f.is_complete())
            release_complete = sum(1 for f in events["RELEASE"] if f.is_complete())
            press_total = len(events["PRESS"])
            release_total = len(events["RELEASE"])

            # Check for asymmetry
            if press_total > 0 and release_total > 0:
                press_rate = press_complete / press_total if press_total > 0 else 0
                release_rate = release_complete / release_total if release_total > 0 else 0

                # Significant difference in completion rates
                if abs(press_rate - release_rate) > 0.2:
                    asymmetric[key_name] = {
                        "press_completion": f"{press_rate*100:.0f}%",
                        "release_completion": f"{release_rate*100:.0f}%",
                        "press_flows": events["PRESS"][:3],  # Sample
                        "release_flows": events["RELEASE"][:3]  # Sample
                    }

        return asymmetric

    def get_layer_skipping_events(self) -> List[EventFlow]:
        """Get events that skip layers (e.g., LAYER1 but not LAYER2)"""
        return [f for f in self.flows if f.layer1 and not f.layer2]


class ReportGenerator:
    """Generates human-readable reports from analysis"""

    def __init__(self, analyzer: FlowAnalyzer, parser: LogParser):
        self.analyzer = analyzer
        self.parser = parser

    def format_flow(self, flow: EventFlow, indent: int = 0) -> str:
        """Format a single event flow for display"""
        prefix = "  " * indent
        lines = []

        if flow.layer1:
            l1 = flow.layer1
            yamy_str = f"0x{l1.yamy_code:04X}" if l1.yamy_code else "NOT FOUND"
            lines.append(f"{prefix}LAYER1: evdev {l1.evdev} ({l1.key_name}) {l1.event_type.value} → {yamy_str}")

        if flow.layer2:
            l2 = flow.layer2
            if l2.is_substitution:
                lines.append(f"{prefix}LAYER2: 0x{l2.input_yamy:04X} → 0x{l2.output_yamy:04X} (SUBST)")
            else:
                lines.append(f"{prefix}LAYER2: 0x{l2.input_yamy:04X} (PASSTHROUGH)")
        else:
            if flow.layer1:
                lines.append(f"{prefix}LAYER2: MISSING")

        if flow.layer3:
            l3 = flow.layer3
            if l3.found:
                lines.append(f"{prefix}LAYER3: 0x{l3.yamy_code:04X} → evdev {l3.evdev} ({l3.key_name}) [{l3.map_source}]")
            else:
                lines.append(f"{prefix}LAYER3: 0x{l3.yamy_code:04X} → NOT FOUND")
        else:
            if flow.layer2 or flow.layer1:
                lines.append(f"{prefix}LAYER3: MISSING")

        return "\n".join(lines)

    def generate_text_report(self) -> str:
        """Generate human-readable text report"""
        lines = ["=" * 80]
        lines.append("YAMY Event Flow Analysis Report")
        lines.append("=" * 80)
        lines.append("")

        # Statistics
        stats = self.analyzer.get_statistics()
        lines.append("STATISTICS")
        lines.append("-" * 80)
        lines.append(f"Total log lines parsed: {self.parser.line_count}")
        lines.append(f"Total events found: {stats['total_events']}")
        lines.append(f"Complete flows (all 3 layers): {stats['complete_flows']}")
        lines.append(f"Incomplete flows: {stats['incomplete_flows']}")
        lines.append(f"Completion rate: {stats['completion_rate']}")
        lines.append(f"PRESS events: {stats['press_events']}")
        lines.append(f"RELEASE events: {stats['release_events']}")
        lines.append("")

        # Missing layers
        missing = self.analyzer.get_missing_layer_events()
        lines.append("MISSING LAYERS")
        lines.append("-" * 80)
        lines.append(f"Events missing Layer 2: {len(missing['missing_layer2'])}")
        lines.append(f"Events missing Layer 3: {len(missing['missing_layer3'])}")
        lines.append(f"Events missing both Layer 2 and 3: {len(missing['missing_both'])}")

        if missing['missing_layer2']:
            lines.append("")
            lines.append("Sample events missing Layer 2:")
            for flow in missing['missing_layer2'][:5]:
                lines.append(self.format_flow(flow, indent=1))
                lines.append("")

        # Asymmetric events
        asymmetric = self.analyzer.get_asymmetric_events()
        if asymmetric:
            lines.append("")
            lines.append("ASYMMETRIC EVENTS (different behavior for PRESS vs RELEASE)")
            lines.append("-" * 80)
            for key_name, data in asymmetric.items():
                lines.append(f"{key_name}: PRESS={data['press_completion']}, RELEASE={data['release_completion']}")
                if data['press_flows']:
                    lines.append("  PRESS sample:")
                    lines.append(self.format_flow(data['press_flows'][0], indent=2))
                if data['release_flows']:
                    lines.append("  RELEASE sample:")
                    lines.append(self.format_flow(data['release_flows'][0], indent=2))
                lines.append("")

        # Complete flow examples
        complete_flows = [f for f in self.analyzer.flows if f.is_complete()]
        if complete_flows:
            lines.append("")
            lines.append("COMPLETE FLOW EXAMPLES")
            lines.append("-" * 80)
            for flow in complete_flows[:3]:
                lines.append(self.format_flow(flow, indent=0))
                lines.append("")

        return "\n".join(lines)

    def generate_json_report(self) -> str:
        """Generate machine-parseable JSON report"""
        stats = self.analyzer.get_statistics()
        missing = self.analyzer.get_missing_layer_events()
        asymmetric = self.analyzer.get_asymmetric_events()

        # Convert flows to dict for JSON serialization
        def flow_to_dict(flow: EventFlow) -> Dict:
            return {
                "layer1": asdict(flow.layer1) if flow.layer1 else None,
                "layer2": asdict(flow.layer2) if flow.layer2 else None,
                "layer3": asdict(flow.layer3) if flow.layer3 else None,
                "complete": flow.is_complete(),
                "asymmetric": flow.is_asymmetric()
            }

        # Convert EventType enums to strings in the dict
        def clean_dict(d):
            if isinstance(d, dict):
                return {k: clean_dict(v) for k, v in d.items()}
            elif isinstance(d, list):
                return [clean_dict(v) for v in d]
            elif isinstance(d, EventType):
                return d.value
            else:
                return d

        report = {
            "statistics": stats,
            "missing_layers": {
                "missing_layer2_count": len(missing['missing_layer2']),
                "missing_layer3_count": len(missing['missing_layer3']),
                "missing_both_count": len(missing['missing_both']),
                "samples": {
                    "missing_layer2": [flow_to_dict(f) for f in missing['missing_layer2'][:5]],
                }
            },
            "asymmetric_events": {
                k: {
                    "press_completion": v["press_completion"],
                    "release_completion": v["release_completion"],
                    "samples": {
                        "press": [flow_to_dict(f) for f in v["press_flows"]],
                        "release": [flow_to_dict(f) for f in v["release_flows"]]
                    }
                }
                for k, v in asymmetric.items()
            }
        }

        return json.dumps(clean_dict(report), indent=2)


def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(
        description="Analyze YAMY debug logs for event flow completeness"
    )
    parser.add_argument(
        "logfile",
        help="Path to debug log file"
    )
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)"
    )
    parser.add_argument(
        "--output",
        "-o",
        help="Output file (default: stdout)"
    )

    args = parser.parse_args()

    # Parse log file
    log_parser = LogParser()
    try:
        log_parser.parse_file(args.logfile)
    except FileNotFoundError:
        print(f"Error: Log file '{args.logfile}' not found", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Error parsing log file: {e}", file=sys.stderr)
        return 1

    # Analyze flows
    analyzer = FlowAnalyzer(log_parser.flows)

    # Generate report
    report_gen = ReportGenerator(analyzer, log_parser)
    if args.format == "json":
        report = report_gen.generate_json_report()
    else:
        report = report_gen.generate_text_report()

    # Output report
    if args.output:
        with open(args.output, 'w') as f:
            f.write(report)
        print(f"Report written to {args.output}")
    else:
        print(report)

    return 0


if __name__ == "__main__":
    sys.exit(main())
