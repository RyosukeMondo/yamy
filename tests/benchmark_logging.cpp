//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// benchmark_logging.cpp - Performance benchmark for Quill logging
//
// This tool measures the hot-path latency for LOG_INFO calls and verifies
// that the <1μs target is met for the 99th percentile.
//
// Requirements (from spec modern-cpp-toolchain task 3.4):
// - Measure hot-path overhead (99th percentile) < 1μs
// - Compare with baseline (printf or direct write)
// - Use RDTSC or high-resolution clock for precision
// - Run enough iterations for statistical significance (10,000+)

#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

#if defined(__x86_64__) || defined(_M_X64)
#include <x86intrin.h>
#define HAS_RDTSC 1
#else
#define HAS_RDTSC 0
#endif

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

#if HAS_RDTSC
// RDTSC-based timing for sub-nanosecond precision
class RdtscTimer {
public:
    RdtscTimer() {
        // Calibrate TSC frequency by measuring cycles over a known time period
        auto start_tsc = __rdtsc();
        auto start_time = high_resolution_clock::now();

        std::this_thread::sleep_for(milliseconds(100));

        auto end_tsc = __rdtsc();
        auto end_time = high_resolution_clock::now();

        auto elapsed_ns = duration_cast<nanoseconds>(end_time - start_time).count();
        auto elapsed_cycles = end_tsc - start_tsc;

        cycles_per_ns_ = static_cast<double>(elapsed_cycles) / elapsed_ns;
    }

    uint64_t start() {
        _mm_lfence();  // Serialize instruction pipeline
        uint64_t tsc = __rdtsc();
        _mm_lfence();
        return tsc;
    }

    double elapsedNs(uint64_t start_tsc) {
        _mm_lfence();
        uint64_t end_tsc = __rdtsc();
        _mm_lfence();

        uint64_t cycles = end_tsc - start_tsc;
        return cycles / cycles_per_ns_;
    }

private:
    double cycles_per_ns_;
};
#endif

// Fallback to high_resolution_clock
class HrTimer {
public:
    high_resolution_clock::time_point start() {
        return high_resolution_clock::now();
    }

    double elapsedNs(high_resolution_clock::time_point start_time) {
        auto end_time = high_resolution_clock::now();
        return duration_cast<nanoseconds>(end_time - start_time).count();
    }
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

void printResults(const std::string& name, const BenchmarkResult& result, double target_us = 1.0) {
    std::cout << "\n" << name << ":\n";
    std::cout << "  Min:    " << std::fixed << std::setprecision(2)
              << result.min_ns << " ns (" << result.min_ns / 1000.0 << " μs)\n";
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

    // Check if requirement is met
    double target_ns = target_us * 1000.0;
    bool meets_requirement = result.p99_ns < target_ns;
    std::cout << "  Status: " << (meets_requirement ? "✓ PASS" : "✗ FAIL")
              << " (requirement: P99 < " << target_us << " μs)\n";
}

template<typename Timer>
BenchmarkResult benchmarkQuillLogging(Timer& timer, int iterations) {
    std::vector<double> latencies;
    latencies.reserve(iterations);

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        LOG_INFO("Warmup message {}", i);
    }

    // Benchmark: measure only the LOG_INFO call (hot path)
    for (int i = 0; i < iterations; ++i) {
        auto start = timer.start();
        LOG_INFO("Benchmark message iteration {} with data: {}, {}, {}", i, 42, 3.14, "test");
        double elapsed = timer.elapsedNs(start);
        latencies.push_back(elapsed);
    }

    return calculateStats(latencies);
}

template<typename Timer>
BenchmarkResult benchmarkPrintfBaseline(Timer& timer, int iterations) {
    std::vector<double> latencies;
    latencies.reserve(iterations);

    // Redirect stdout to /dev/null for fair comparison
    FILE* devnull = fopen("/dev/null", "w");
    if (!devnull) {
        std::cerr << "Warning: Could not open /dev/null, printf will write to stdout\n";
        devnull = stdout;
    }

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        fprintf(devnull, "Warmup message %d\n", i);
    }

    // Benchmark
    for (int i = 0; i < iterations; ++i) {
        auto start = timer.start();
        fprintf(devnull, "Benchmark message iteration %d with data: %d, %f, %s\n",
                i, 42, 3.14, "test");
        double elapsed = timer.elapsedNs(start);
        latencies.push_back(elapsed);
    }

    if (devnull != stdout) {
        fclose(devnull);
    }

    return calculateStats(latencies);
}

int main() {
    std::cout << "=============================================================\n";
    std::cout << "Quill Logging Performance Benchmark\n";
    std::cout << "=============================================================\n";

#if HAS_RDTSC
    std::cout << "Timer: RDTSC (calibrated)\n";
    RdtscTimer timer;
#else
    std::cout << "Timer: std::chrono::high_resolution_clock\n";
    HrTimer timer;
#endif

    std::cout << "Configuration:\n";
    std::cout << "  Warmup iterations:    " << WARMUP_ITERATIONS << "\n";
    std::cout << "  Benchmark iterations: " << BENCHMARK_ITERATIONS << "\n";
    std::cout << "  Target latency:       < 1 μs (P99)\n\n";

    // Initialize Quill logger
    yamy::log::init();

    std::cout << "Running benchmarks...\n";

    // Benchmark Quill LOG_INFO
    auto quill_result = benchmarkQuillLogging(timer, BENCHMARK_ITERATIONS);
    printResults("Quill LOG_INFO (hot path only)", quill_result, 1.0);

    // Flush logs before baseline test
    yamy::log::flush();

    // Benchmark printf baseline
    auto printf_result = benchmarkPrintfBaseline(timer, BENCHMARK_ITERATIONS);
    printResults("printf baseline (to /dev/null)", printf_result, -1.0);  // No requirement for printf

    // Comparison
    std::cout << "\n=============================================================\n";
    std::cout << "Comparison (P99 latency):\n";
    std::cout << "  Quill:   " << std::fixed << std::setprecision(2)
              << quill_result.p99_ns / 1000.0 << " μs\n";
    std::cout << "  printf:  " << printf_result.p99_ns / 1000.0 << " μs\n";

    if (quill_result.p99_ns < printf_result.p99_ns) {
        double speedup = printf_result.p99_ns / quill_result.p99_ns;
        std::cout << "  Quill is " << speedup << "x faster than printf\n";
    } else {
        double slowdown = quill_result.p99_ns / printf_result.p99_ns;
        std::cout << "  Quill is " << slowdown << "x slower than printf\n";
        std::cout << "  Note: Quill provides async logging with structured output,\n";
        std::cout << "        which printf doesn't. The comparison is for reference only.\n";
    }

    std::cout << "=============================================================\n";

    // Final status
    bool success = quill_result.p99_ns < 1000.0;  // < 1 μs
    if (success) {
        std::cout << "\n✓ SUCCESS: Quill meets the <1μs P99 latency requirement\n";
        return 0;
    } else {
        std::cout << "\n✗ FAILED: Quill does not meet the <1μs P99 latency requirement\n";
        std::cout << "  Actual P99: " << quill_result.p99_ns / 1000.0 << " μs\n";
        return 1;
    }
}
