//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// benchmark_json_loader.cpp - Performance benchmark for JSON config loading
//
// This tool measures JSON config loading latency to verify that configs
// load in <10ms (requirement NFR-1 from json-refactoring spec).

#include "../src/core/settings/json_config_loader.h"
#include "../src/core/settings/setting.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <vector>
#include <filesystem>

using namespace yamy::settings;
using namespace std::chrono;

// Test configuration
constexpr int WARMUP_ITERATIONS = 10;
constexpr int BENCHMARK_ITERATIONS = 1000;

struct BenchmarkResult {
    double min_ms;
    double max_ms;
    double mean_ms;
    double median_ms;
    double p95_ms;
    double p99_ms;
};

BenchmarkResult calculateStats(std::vector<double>& latencies) {
    std::sort(latencies.begin(), latencies.end());

    BenchmarkResult result;
    result.min_ms = latencies.front();
    result.max_ms = latencies.back();
    result.mean_ms = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    result.median_ms = latencies[latencies.size() / 2];
    result.p95_ms = latencies[static_cast<size_t>(latencies.size() * 0.95)];
    result.p99_ms = latencies[static_cast<size_t>(latencies.size() * 0.99)];

    return result;
}

void printResults(const std::string& name, const BenchmarkResult& result, double target_ms) {
    std::cout << "\n" << name << ":\n";
    std::cout << "  Min:    " << std::fixed << std::setprecision(3) << result.min_ms << " ms\n";
    std::cout << "  Mean:   " << result.mean_ms << " ms\n";
    std::cout << "  Median: " << result.median_ms << " ms\n";
    std::cout << "  P95:    " << result.p95_ms << " ms\n";
    std::cout << "  P99:    " << result.p99_ms << " ms\n";
    std::cout << "  Max:    " << result.max_ms << " ms\n";

    bool meets_requirement = result.p99_ms < target_ms;
    std::cout << "  Status: " << (meets_requirement ? "✓ PASS" : "✗ FAIL")
              << " (requirement: P99 < " << target_ms << "ms)\n";
}

BenchmarkResult benchmarkConfigLoad(const std::string& config_path, const std::string& name) {
    std::cout << "\n=============================================================\n";
    std::cout << "Benchmarking: " << name << "\n";
    std::cout << "Config: " << config_path << "\n";
    std::cout << "=============================================================\n";

    // Check if file exists
    if (!std::filesystem::exists(config_path)) {
        std::cerr << "Error: Config file not found: " << config_path << "\n";
        return {0, 0, 0, 0, 0, 0};
    }

    JsonConfigLoader loader(nullptr);  // No logging for benchmarks

    std::cout << "\nConfiguration:\n";
    std::cout << "  Warmup iterations:    " << WARMUP_ITERATIONS << "\n";
    std::cout << "  Benchmark iterations: " << BENCHMARK_ITERATIONS << "\n";

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        Setting setting;
        loader.load(&setting, config_path);
    }

    // Benchmark
    std::vector<double> latencies;
    latencies.reserve(BENCHMARK_ITERATIONS);

    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        Setting setting;

        auto start = high_resolution_clock::now();
        bool success = loader.load(&setting, config_path);
        auto end = high_resolution_clock::now();

        if (!success) {
            std::cerr << "Warning: Load failed on iteration " << i << "\n";
        }

        double elapsed_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
        latencies.push_back(elapsed_ms);
    }

    BenchmarkResult result = calculateStats(latencies);
    printResults(name, result, 10.0);  // 10ms target

    return result;
}

int main(int argc, char** argv) {
    std::cout << "=============================================================\n";
    std::cout << "JSON Config Loader Performance Benchmark\n";
    std::cout << "=============================================================\n";

    // Benchmark different config files
    std::vector<std::pair<std::string, std::string>> configs = {
        {"keymaps/config.json", "Basic Config"},
        {"keymaps/vim-mode.json", "Vim Mode Config"},
        {"keymaps/emacs-mode.json", "Emacs Mode Config"}
    };

    std::vector<BenchmarkResult> results;
    bool all_pass = true;

    for (const auto& [path, name] : configs) {
        BenchmarkResult result = benchmarkConfigLoad(path, name);
        results.push_back(result);

        if (result.p99_ms >= 10.0) {
            all_pass = false;
        }
    }

    // Summary
    std::cout << "\n=============================================================\n";
    std::cout << "Summary\n";
    std::cout << "=============================================================\n";

    std::cout << "\nPerformance Requirements:\n";
    for (size_t i = 0; i < configs.size(); i++) {
        std::cout << "  [" << (results[i].p99_ms < 10.0 ? "✓" : "✗")
                  << "] " << configs[i].second << " P99 < 10ms ("
                  << std::fixed << std::setprecision(3) << results[i].p99_ms << " ms)\n";
    }

    std::cout << "\n" << (all_pass ? "✓ ALL REQUIREMENTS MET" : "✗ SOME REQUIREMENTS FAILED") << "\n\n";

    return all_pass ? 0 : 1;
}
