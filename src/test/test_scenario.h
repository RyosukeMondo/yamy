#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// test_scenario.h - Test scenario data structures

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <map>

namespace yamy::test {

/// Event type
enum class EventType {
    PRESS,
    RELEASE
};

/// Single key event for testing
struct KeyEvent {
    uint16_t evdev_code;
    std::string key_name;
    EventType type;
    uint32_t delay_before_ms;  // Delay before this event

    KeyEvent() : evdev_code(0), type(EventType::PRESS), delay_before_ms(0) {}
};

/// Captured output event with timing
struct CapturedEvent {
    uint16_t evdev_code;
    std::string key_name;
    EventType type;
    std::chrono::microseconds timestamp;
    std::chrono::microseconds latency;

    CapturedEvent()
        : evdev_code(0)
        , type(EventType::PRESS)
        , timestamp(0)
        , latency(0) {}
};

/// Individual test case within a scenario
struct TestCase {
    std::string name;
    std::string description;
    std::vector<KeyEvent> input;
    std::vector<KeyEvent> expected_output;
    uint32_t timeout_ms;
    uint32_t max_latency_us;

    TestCase() : timeout_ms(1000), max_latency_us(1000) {}
};

/// Test execution result
enum class TestStatus {
    PASSED,
    FAILED,
    TIMEOUT,
    ERROR
};

/// Test case result
struct TestCaseResult {
    std::string name;
    TestStatus status;
    uint32_t duration_ms;
    uint32_t latency_us;
    std::vector<CapturedEvent> actual_output;
    std::string error_message;

    TestCaseResult()
        : status(TestStatus::ERROR)
        , duration_ms(0)
        , latency_us(0) {}
};

/// Test scenario setup configuration
struct ScenarioSetup {
    std::vector<std::string> daemon_args;
    std::map<std::string, std::string> env_vars;

    ScenarioSetup() = default;
};

/// Complete test scenario
struct TestScenario {
    std::string name;
    std::string description;
    std::string config_file;  // Path to .mayu config
    ScenarioSetup setup;
    std::vector<TestCase> test_cases;

    TestScenario() = default;
};

/// Test scenario result
struct ScenarioResult {
    std::string scenario_name;
    TestStatus status;
    uint32_t duration_ms;
    std::vector<TestCaseResult> test_case_results;

    ScenarioResult() : status(TestStatus::ERROR), duration_ms(0) {}
};

/// Test suite configuration
struct TestSuite {
    std::string name;
    std::string description;
    std::vector<std::string> scenario_files;
    bool build_daemon;
    bool clean_state;
    bool collect_logs;

    TestSuite()
        : build_daemon(false)
        , clean_state(false)
        , collect_logs(false) {}
};

/// Test suite result summary
struct TestSuiteResult {
    std::string suite_name;
    std::string timestamp;
    uint32_t total_scenarios;
    uint32_t total_test_cases;
    uint32_t passed;
    uint32_t failed;
    uint32_t duration_ms;
    std::vector<ScenarioResult> scenario_results;

    TestSuiteResult()
        : total_scenarios(0)
        , total_test_cases(0)
        , passed(0)
        , failed(0)
        , duration_ms(0) {}
};

} // namespace yamy::test
