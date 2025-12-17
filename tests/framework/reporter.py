
import json
from dataclasses import asdict
from typing import List
from .defs import TestResult

class ReportGenerator:
    def __init__(self, report_path: str = 'test_report.json'):
        self.report_path = report_path
        self.results: List[TestResult] = []

    def add_result(self, result: TestResult):
        """Adds a single test result to the report."""
        self.results.append(result)

    def generate_summary(self):
        """Prints a summary of the test results to the console."""
        passes = sum(1 for r in self.results if r.passed)
        fails = len(self.results) - passes

        print("\n=== Test Summary ===")
        print(f"  Total Tests: {len(self.results)}")
        print(f"  Passed:      {passes}")
        print(f"  Failed:      {fails}")
        print("====================")

        if fails > 0:
            print("\n--- Failed Tests ---")
            for result in self.results:
                if not result.passed:
                    print(f"- Input: {result.mapping.input_key}, "
                          f"Expected: {result.mapping.output_key}, "
                          f"Got: {result.actual_evdev}, "
                          f"Error: {result.error_message}")
            print("--------------------")
            
    def save_json(self):
        """Saves the collected test results to a JSON file."""
        print(f"\n[ReportGenerator] Saving report to {self.report_path}...")
        
        report_data = {
            'summary': {
                'total': len(self.results),
                'passed': sum(1 for r in self.results if r.passed),
                'failed': sum(1 for r in self.results if not r.passed),
            },
            'results': [asdict(r) for r in self.results]
        }
        
        try:
            with open(self.report_path, 'w', encoding='utf-8') as f:
                json.dump(report_data, f, indent=4)
            print(f"[ReportGenerator] Report saved successfully.")
        except IOError as e:
            print(f"[ReportGenerator] ERROR: Failed to save report: {e}")
