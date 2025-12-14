//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// benchmark_event_processor.cpp - Performance benchmark for EventProcessor
//
// This tool measures the event processing latency for all 3 layers and
// verifies that performance requirements are met (< 1ms per event).

#include "../src/core/engine/engine_event_processor.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <map>
#include <numeric>
#include <vector>
#include <cstdlib>

using namespace yamy;
using namespace std::chrono;

// Test configuration
constexpr int WARMUP_ITERATIONS = 1000;
constexpr int BENCHMARK_ITERATIONS = 100000;

struct BenchmarkResult {
    double min_ns;
    double max_ns;
    double mean_ns;
    double median_ns;
    double p95_ns;
    double p99_ns;
};

BenchmarkResult calculateStats(std::vector<double>& latencies) {
    std::sort(latencies.begin(), latencies.end());

    BenchmarkResult result;
    result.min_ns = latencies.front();
    result.max_ns = latencies.back();
    result.mean_ns = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    result.median_ns = latencies[latencies.size() / 2];
    result.p95_ns = latencies[static_cast<size_t>(latencies.size() * 0.95)];
    result.p99_ns = latencies[static_cast<size_t>(latencies.size() * 0.99)];

    return result;
}

void printResults(const std::string& name, const BenchmarkResult& result) {
    std::cout << "\n" << name << ":\n";
    std::cout << "  Min:    " << std::fixed << std::setprecision(2) << result.min_ns << " ns ("
              << result.min_ns / 1000.0 << " μs)\n";
    std::cout << "  Mean:   " << result.mean_ns << " ns ("
              << result.mean_ns / 1000.0 << " μs)\n";
    std::cout << "  Median: " << result.median_ns << " ns ("
              << result.median_ns / 1000.0 << " μs)\n";
    std::cout << "  P95:    " << result.p95_ns << " ns ("
              << result.p95_ns / 1000.0 << " μs)\n";
    std::cout << "  P99:    " << result.p99_ns << " ns ("
              << result.p99_ns / 1000.0 << " μs)\n";
    std::cout << "  Max:    " << result.max_ns << " ns ("
              << result.max_ns / 1000.0 << " μs)\n";

    // Check if requirement is met (< 1ms = 1,000,000 ns)
    bool meets_requirement = result.p99_ns < 1000000.0;
    std::cout << "  Status: " << (meets_requirement ? "✓ PASS" : "✗ FAIL")
              << " (requirement: P99 < 1ms)\n";
}

int main(int argc, char** argv) {
    std::cout << "=============================================================\n";
    std::cout << "EventProcessor Performance Benchmark\n";
    std::cout << "=============================================================\n";

    // Disable debug logging for accurate benchmarking
    unsetenv("YAMY_DEBUG_KEYCODE");

    // Create a simple substitution table for testing
    // W→A mapping (evdev 17 → 0x0011 → 0x001E → evdev 30)
    SubstitutionTable subst_table;
    subst_table[0x0011] = 0x001E;  // W → A
    subst_table[0x0031] = 0x002A;  // N → LShift (VK_LSHIFT)

    // Initialize EventProcessor
    EventProcessor processor(subst_table);

    std::cout << "\nConfiguration:\n";
    std::cout << "  Warmup iterations:    " << WARMUP_ITERATIONS << "\n";
    std::cout << "  Benchmark iterations: " << BENCHMARK_ITERATIONS << "\n";
    std::cout << "  Debug logging:        DISABLED\n";

    // Test cases: various key events
    struct TestCase {
        std::string name;
        uint16_t evdev;
        EventType type;
    };

    std::vector<TestCase> test_cases = {
        {"W key PRESS (with substitution)", 17, EventType::PRESS},
        {"W key RELEASE (with substitution)", 17, EventType::RELEASE},
        {"N key PRESS (modifier substitution)", 49, EventType::PRESS},
        {"A key PRESS (no substitution)", 30, EventType::PRESS},
    };

    for (const auto& test : test_cases) {
        std::cout << "\n-------------------------------------------------------------\n";
        std::cout << "Benchmarking: " << test.name << "\n";
        std::cout << "-------------------------------------------------------------\n";

        // Warmup
        for (int i = 0; i < WARMUP_ITERATIONS; i++) {
            processor.processEvent(test.evdev, test.type, nullptr);
        }

        // Benchmark
        std::vector<double> latencies;
        latencies.reserve(BENCHMARK_ITERATIONS);

        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            auto start = high_resolution_clock::now();
            processor.processEvent(test.evdev, test.type, nullptr);
            auto end = high_resolution_clock::now();

            double elapsed_ns = duration_cast<nanoseconds>(end - start).count();
            latencies.push_back(elapsed_ns);
        }

        BenchmarkResult result = calculateStats(latencies);
        printResults(test.name, result);
    }

    // Now benchmark WITH debug logging enabled
    std::cout << "\n=============================================================\n";
    std::cout << "Logging Overhead Benchmark\n";
    std::cout << "=============================================================\n";

    setenv("YAMY_DEBUG_KEYCODE", "1", 1);
    EventProcessor processor_with_logging(subst_table);

    std::cout << "\nRe-running W key PRESS with debug logging enabled...\n";

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        processor_with_logging.processEvent(17, EventType::PRESS, nullptr);
    }

    // Benchmark
    std::vector<double> latencies_with_logging;
    latencies_with_logging.reserve(BENCHMARK_ITERATIONS);

    // Suppress output during benchmark
    std::cout << "\nRunning " << BENCHMARK_ITERATIONS << " iterations with logging...\n";

    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        auto start = high_resolution_clock::now();
        processor_with_logging.processEvent(17, EventType::PRESS, nullptr);
        auto end = high_resolution_clock::now();

        double elapsed_ns = duration_cast<nanoseconds>(end - start).count();
        latencies_with_logging.push_back(elapsed_ns);
    }

    BenchmarkResult result_with_logging = calculateStats(latencies_with_logging);

    // Compare overhead
    std::vector<double> baseline_latencies;
    baseline_latencies.reserve(BENCHMARK_ITERATIONS);

    unsetenv("YAMY_DEBUG_KEYCODE");
    EventProcessor processor_no_logging(subst_table);

    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        processor_no_logging.processEvent(17, EventType::PRESS, nullptr);
    }

    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        auto start = high_resolution_clock::now();
        processor_no_logging.processEvent(17, EventType::PRESS, nullptr);
        auto end = high_resolution_clock::now();

        double elapsed_ns = duration_cast<nanoseconds>(end - start).count();
        baseline_latencies.push_back(elapsed_ns);
    }

    BenchmarkResult baseline = calculateStats(baseline_latencies);

    std::cout << "\n-------------------------------------------------------------\n";
    std::cout << "Logging Overhead Analysis\n";
    std::cout << "-------------------------------------------------------------\n";

    printResults("Without logging", baseline);
    printResults("With logging", result_with_logging);

    double overhead_percent = ((result_with_logging.mean_ns - baseline.mean_ns) / baseline.mean_ns) * 100.0;

    std::cout << "\nOverhead:\n";
    std::cout << "  Absolute: " << std::fixed << std::setprecision(2)
              << (result_with_logging.mean_ns - baseline.mean_ns) << " ns\n";
    std::cout << "  Relative: " << overhead_percent << "%\n";
    std::cout << "  Status:   " << (overhead_percent < 10.0 ? "✓ PASS" : "✗ FAIL")
              << " (requirement: < 10%)\n";

    std::cout << "\n=============================================================\n";
    std::cout << "Summary\n";
    std::cout << "=============================================================\n";

    std::cout << "\nPerformance Requirements:\n";
    std::cout << "  [" << (baseline.p99_ns < 1000000.0 ? "✓" : "✗")
              << "] Event processing latency P99 < 1ms ("
              << baseline.p99_ns / 1000000.0 << " ms)\n";
    std::cout << "  [" << (overhead_percent < 10.0 ? "✓" : "✗")
              << "] Logging overhead < 10% ("
              << overhead_percent << "%)\n";

    bool all_pass = (baseline.p99_ns < 1000000.0) && (overhead_percent < 10.0);

    std::cout << "\n" << (all_pass ? "✓ ALL REQUIREMENTS MET" : "✗ SOME REQUIREMENTS FAILED") << "\n\n";

    return all_pass ? 0 : 1;
}
