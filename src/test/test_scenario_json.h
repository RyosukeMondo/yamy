#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// test_scenario_json.h - JSON parsing and serialization for test scenarios

#include "test_scenario.h"
#include <string>

namespace yamy::test {

/// Convert EventType to string
const char* eventTypeToString(EventType type);

/// Convert string to EventType
EventType stringToEventType(const std::string& str);

/// Convert TestStatus to string
const char* testStatusToString(TestStatus status);

/// Load test scenario from JSON file
TestScenario loadScenarioFromJson(const std::string& filename);

/// Load test suite from JSON file
TestSuite loadSuiteFromJson(const std::string& filename);

/// Serialize captured events to JSON string
std::string serializeCapturedEvents(const std::vector<CapturedEvent>& events);

/// Serialize test case result to JSON string
std::string serializeTestCaseResult(const TestCaseResult& result);

/// Serialize scenario result to JSON string
std::string serializeScenarioResult(const ScenarioResult& result);

/// Serialize test suite result to JSON string
std::string serializeTestSuiteResult(const TestSuiteResult& result);

} // namespace yamy::test
