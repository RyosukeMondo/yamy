//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// test_scenario_json.cpp - JSON parsing and serialization for test scenarios

#include "test_scenario.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

namespace yamy::test {

// Helper: Convert EventType to/from string
const char* eventTypeToString(EventType type) {
    return (type == EventType::PRESS) ? "press" : "release";
}

EventType stringToEventType(const std::string& str) {
    if (str == "press") return EventType::PRESS;
    if (str == "release") return EventType::RELEASE;
    throw std::runtime_error("Invalid event type: " + str);
}

const char* testStatusToString(TestStatus status) {
    switch (status) {
        case TestStatus::PASSED: return "PASSED";
        case TestStatus::FAILED: return "FAILED";
        case TestStatus::TIMEOUT: return "TIMEOUT";
        case TestStatus::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

// Parse KeyEvent from JSON
KeyEvent parseKeyEvent(const json& j) {
    KeyEvent event;

    if (j.contains("evdev_code")) {
        event.evdev_code = j["evdev_code"].get<uint16_t>();
    } else if (j.contains("evdev")) {
        event.evdev_code = j["evdev"].get<uint16_t>();
    }

    if (j.contains("key_name")) {
        event.key_name = j["key_name"].get<std::string>();
    }

    if (j.contains("type")) {
        event.type = stringToEventType(j["type"].get<std::string>());
    }

    if (j.contains("delay_before_ms")) {
        event.delay_before_ms = j["delay_before_ms"].get<uint32_t>();
    } else if (j.contains("delay_ms")) {
        event.delay_before_ms = j["delay_ms"].get<uint32_t>();
    }

    return event;
}

// Parse TestCase from JSON
TestCase parseTestCase(const json& j) {
    TestCase test_case;

    if (j.contains("name")) {
        test_case.name = j["name"].get<std::string>();
    }

    if (j.contains("description")) {
        test_case.description = j["description"].get<std::string>();
    }

    if (j.contains("input") && j["input"].is_array()) {
        for (const auto& event_json : j["input"]) {
            test_case.input.push_back(parseKeyEvent(event_json));
        }
    }

    if (j.contains("expected_output") && j["expected_output"].is_array()) {
        for (const auto& event_json : j["expected_output"]) {
            test_case.expected_output.push_back(parseKeyEvent(event_json));
        }
    }

    if (j.contains("timeout_ms")) {
        test_case.timeout_ms = j["timeout_ms"].get<uint32_t>();
    }

    if (j.contains("max_latency_us")) {
        test_case.max_latency_us = j["max_latency_us"].get<uint32_t>();
    }

    return test_case;
}

// Parse ScenarioSetup from JSON
ScenarioSetup parseScenarioSetup(const json& j) {
    ScenarioSetup setup;

    if (j.contains("daemon_args") && j["daemon_args"].is_array()) {
        for (const auto& arg : j["daemon_args"]) {
            setup.daemon_args.push_back(arg.get<std::string>());
        }
    }

    if (j.contains("env") && j["env"].is_object()) {
        for (auto it = j["env"].begin(); it != j["env"].end(); ++it) {
            setup.env_vars[it.key()] = it.value().get<std::string>();
        }
    }

    return setup;
}

// Load TestScenario from JSON file
TestScenario loadScenarioFromJson(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open scenario file: " + filename);
    }

    json j;
    file >> j;

    TestScenario scenario;

    if (j.contains("name")) {
        scenario.name = j["name"].get<std::string>();
    }

    if (j.contains("description")) {
        scenario.description = j["description"].get<std::string>();
    }

    if (j.contains("config")) {
        scenario.config_file = j["config"].get<std::string>();
    }

    if (j.contains("setup")) {
        scenario.setup = parseScenarioSetup(j["setup"]);
    }

    if (j.contains("test_cases") && j["test_cases"].is_array()) {
        for (const auto& tc_json : j["test_cases"]) {
            scenario.test_cases.push_back(parseTestCase(tc_json));
        }
    }

    return scenario;
}

// Load TestSuite from JSON file
TestSuite loadSuiteFromJson(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open suite file: " + filename);
    }

    json j;
    file >> j;

    TestSuite suite;

    if (j.contains("name")) {
        suite.name = j["name"].get<std::string>();
    }

    if (j.contains("description")) {
        suite.description = j["description"].get<std::string>();
    }

    if (j.contains("test_scenarios") && j["test_scenarios"].is_array()) {
        for (const auto& scenario_file : j["test_scenarios"]) {
            suite.scenario_files.push_back(scenario_file.get<std::string>());
        }
    }

    if (j.contains("global_setup")) {
        const auto& setup = j["global_setup"];
        if (setup.contains("build_daemon")) {
            suite.build_daemon = setup["build_daemon"].get<bool>();
        }
        if (setup.contains("clean_state")) {
            suite.clean_state = setup["clean_state"].get<bool>();
        }
    }

    if (j.contains("global_teardown")) {
        const auto& teardown = j["global_teardown"];
        if (teardown.contains("collect_logs")) {
            suite.collect_logs = teardown["collect_logs"].get<bool>();
        }
    }

    return suite;
}

// Serialize CapturedEvent to JSON
json serializeCapturedEvent(const CapturedEvent& event) {
    return json{
        {"evdev_code", event.evdev_code},
        {"key_name", event.key_name},
        {"type", eventTypeToString(event.type)},
        {"timestamp_us", event.timestamp.count()},
        {"latency_us", event.latency.count()}
    };
}

// Serialize captured events to JSON string
std::string serializeCapturedEvents(const std::vector<CapturedEvent>& events) {
    json j = json::object();
    json events_array = json::array();

    for (const auto& event : events) {
        events_array.push_back(serializeCapturedEvent(event));
    }

    j["captured_events"] = events_array;

    // Add summary
    if (!events.empty()) {
        uint64_t total_latency = 0;
        for (const auto& event : events) {
            total_latency += event.latency.count();
        }

        auto first_ts = events.front().timestamp;
        auto last_ts = events.back().timestamp;
        auto duration_us = (last_ts - first_ts).count();

        j["summary"] = {
            {"total_events", events.size()},
            {"duration_us", duration_us},
            {"average_latency_us", total_latency / events.size()}
        };
    }

    return j.dump(2);  // Pretty print with 2-space indent
}

// Serialize TestCaseResult to JSON (internal)
json serializeTestCaseResultToJson(const TestCaseResult& result) {
    json j = {
        {"name", result.name},
        {"status", testStatusToString(result.status)},
        {"duration_ms", result.duration_ms},
        {"latency_us", result.latency_us}
    };

    if (!result.error_message.empty()) {
        j["error"] = result.error_message;
    }

    // Add actual output if available
    if (!result.actual_output.empty()) {
        json actual_array = json::array();
        for (const auto& event : result.actual_output) {
            actual_array.push_back(serializeCapturedEvent(event));
        }
        j["actual_output"] = actual_array;
    }

    return j;
}

// Serialize TestCaseResult to JSON string
std::string serializeTestCaseResult(const TestCaseResult& result) {
    return serializeTestCaseResultToJson(result).dump(2);
}

// Serialize ScenarioResult to JSON (internal)
json serializeScenarioResultToJson(const ScenarioResult& result) {
    json j = {
        {"scenario", result.scenario_name},
        {"status", testStatusToString(result.status)},
        {"duration_ms", result.duration_ms}
    };

    json test_cases_array = json::array();
    for (const auto& tc_result : result.test_case_results) {
        test_cases_array.push_back(serializeTestCaseResultToJson(tc_result));
    }
    j["test_cases"] = test_cases_array;

    return j;
}

// Serialize ScenarioResult to JSON string
std::string serializeScenarioResult(const ScenarioResult& result) {
    return serializeScenarioResultToJson(result).dump(2);
}

// Serialize TestSuiteResult to JSON string
std::string serializeTestSuiteResult(const TestSuiteResult& result) {
    json j = {
        {"suite", result.suite_name},
        {"timestamp", result.timestamp},
        {"summary", {
            {"total_scenarios", result.total_scenarios},
            {"total_test_cases", result.total_test_cases},
            {"passed", result.passed},
            {"failed", result.failed},
            {"duration_ms", result.duration_ms}
        }}
    };

    json results_array = json::array();
    for (const auto& scenario_result : result.scenario_results) {
        results_array.push_back(serializeScenarioResultToJson(scenario_result));
    }
    j["results"] = results_array;

    return j.dump(2);
}

} // namespace yamy::test
