//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// benchmark_modal_modifier.cpp - Performance benchmark for modal modifier pipeline
//
// This benchmark measures latency for all components of the modal modifier system:
// - Hold detection (ModifierKeyHandler::processNumberKey)
// - Modifier state update (ModifierState::activate/deactivate)
// - Keymap lookup with modal modifiers (if integrated)
// - Full pipeline (evdev → uinput simulation)
//
// Performance targets:
// - Hold detection P99: < 10μs
// - State update P99: < 5μs
// - Keymap lookup P99: < 15μs (if applicable)
// - Full pipeline P99: < 1ms

#include "core/engine/modifier_key_handler.h"
#include "core/engine/engine_event_processor.h"
#include "core/input/modifier_state.h"
#include "core/input/keyboard.h"
#include "platform/types.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <map>
#include <numeric>
#include <vector>
#include <cstdlib>
#include <thread>

using namespace yamy;
using namespace yamy::engine;
using namespace yamy::input;
using namespace std::chrono;

// Test configuration
constexpr int WARMUP_ITERATIONS = 1000;
constexpr int BENCHMARK_ITERATIONS = 100000;

struct BenchmarkResult {
    double min_ns;
    double max_ns;
    double mean_ns;
    double median_ns;
    double p50_ns;
    double p95_ns;
    double p99_ns;
    double p999_ns;
};

BenchmarkResult calculateStats(std::vector<double>& latencies) {
    std::sort(latencies.begin(), latencies.end());

    BenchmarkResult result;
    result.min_ns = latencies.front();
    result.max_ns = latencies.back();
    result.mean_ns = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    result.median_ns = latencies[latencies.size() / 2];
    result.p50_ns = latencies[static_cast<size_t>(latencies.size() * 0.50)];
    result.p95_ns = latencies[static_cast<size_t>(latencies.size() * 0.95)];
    result.p99_ns = latencies[static_cast<size_t>(latencies.size() * 0.99)];
    result.p999_ns = latencies[static_cast<size_t>(latencies.size() * 0.999)];

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
    std::cout << "  P50:    " << result.p50_ns << " ns ("
              << result.p50_ns / 1000.0 << " μs)\n";
    std::cout << "  P95:    " << result.p95_ns << " ns ("
              << result.p95_ns / 1000.0 << " μs)\n";
    std::cout << "  P99:    " << result.p99_ns << " ns ("
              << result.p99_ns / 1000.0 << " μs)\n";
    std::cout << "  P99.9:  " << result.p999_ns << " ns ("
              << result.p999_ns / 1000.0 << " μs)\n";
    std::cout << "  Max:    " << result.max_ns << " ns ("
              << result.max_ns / 1000.0 << " μs)\n";
}

void printResultsWithTarget(const std::string& name, const BenchmarkResult& result, double target_us) {
    printResults(name, result);
    bool meets_requirement = result.p99_ns < (target_us * 1000.0);
    std::cout << "  Status: " << (meets_requirement ? "✓ PASS" : "✗ FAIL")
              << " (requirement: P99 < " << target_us << "μs)\n";
}

void outputCSV(const std::string& filename, const std::map<std::string, BenchmarkResult>& results) {
    std::cout << "\n=============================================================\n";
    std::cout << "CSV Output (save to " << filename << ")\n";
    std::cout << "=============================================================\n";

    std::cout << "Component,P50,P95,P99,P99.9\n";
    for (const auto& [name, result] : results) {
        std::cout << name << ","
                  << static_cast<int>(result.p50_ns) << ","
                  << static_cast<int>(result.p95_ns) << ","
                  << static_cast<int>(result.p99_ns) << ","
                  << static_cast<int>(result.p999_ns) << "\n";
    }
}

//=============================================================================
// Benchmark 1: Hold Detection (ModifierKeyHandler::processNumberKey)
//=============================================================================

BenchmarkResult benchmarkHoldDetection() {
    std::cout << "\n=============================================================\n";
    std::cout << "Benchmark 1: Hold Detection (processNumberKey)\n";
    std::cout << "=============================================================\n";

    ModifierKeyHandler handler(200);  // 200ms threshold

    // Register number key 1 (scancode 0x0002) as LShift
    handler.registerNumberModifier(0x0002, HardwareModifier::LSHIFT);

    std::cout << "\nConfiguration:\n";
    std::cout << "  Registered modifier: 0x0002 (_1) → LSHIFT\n";
    std::cout << "  Hold threshold:      200ms\n";
    std::cout << "  Warmup iterations:   " << WARMUP_ITERATIONS << "\n";
    std::cout << "  Benchmark iterations:" << BENCHMARK_ITERATIONS << "\n";

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        handler.processNumberKey(0x0002, EventType::PRESS);
        handler.processNumberKey(0x0002, EventType::RELEASE);
    }

    // Benchmark: PRESS event (IDLE → WAITING transition)
    std::vector<double> latencies;
    latencies.reserve(BENCHMARK_ITERATIONS);

    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        auto start = high_resolution_clock::now();
        handler.processNumberKey(0x0002, EventType::PRESS);
        auto end = high_resolution_clock::now();

        double elapsed_ns = duration_cast<nanoseconds>(end - start).count();
        latencies.push_back(elapsed_ns);

        // Reset state for next iteration
        handler.processNumberKey(0x0002, EventType::RELEASE);
    }

    BenchmarkResult result = calculateStats(latencies);
    printResultsWithTarget("Hold Detection (PRESS → WAITING)", result, 10.0);

    return result;
}

//=============================================================================
// Benchmark 2: Modifier State Update
//=============================================================================

BenchmarkResult benchmarkModifierStateActivate() {
    std::cout << "\n=============================================================\n";
    std::cout << "Benchmark 2: Modifier State Activation\n";
    std::cout << "=============================================================\n";

    ModifierState modState;

    std::cout << "\nConfiguration:\n";
    std::cout << "  Test: activate(Modifier::Type_Mod9)\n";
    std::cout << "  Warmup iterations:   " << WARMUP_ITERATIONS << "\n";
    std::cout << "  Benchmark iterations:" << BENCHMARK_ITERATIONS << "\n";

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        modState.activate(Modifier::Type_Mod9);
        modState.deactivate(Modifier::Type_Mod9);
    }

    // Benchmark activate()
    std::vector<double> latencies;
    latencies.reserve(BENCHMARK_ITERATIONS);

    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        modState.deactivate(Modifier::Type_Mod9);  // Reset

        auto start = high_resolution_clock::now();
        modState.activate(Modifier::Type_Mod9);
        auto end = high_resolution_clock::now();

        double elapsed_ns = duration_cast<nanoseconds>(end - start).count();
        latencies.push_back(elapsed_ns);
    }

    BenchmarkResult result = calculateStats(latencies);
    printResultsWithTarget("Modifier State Activate", result, 5.0);

    return result;
}

BenchmarkResult benchmarkModifierStateDeactivate() {
    std::cout << "\n-------------------------------------------------------------\n";
    std::cout << "Modifier State Deactivation\n";
    std::cout << "-------------------------------------------------------------\n";

    ModifierState modState;

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        modState.activate(Modifier::Type_Mod9);
        modState.deactivate(Modifier::Type_Mod9);
    }

    // Benchmark deactivate()
    std::vector<double> latencies;
    latencies.reserve(BENCHMARK_ITERATIONS);

    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        modState.activate(Modifier::Type_Mod9);  // Set

        auto start = high_resolution_clock::now();
        modState.deactivate(Modifier::Type_Mod9);
        auto end = high_resolution_clock::now();

        double elapsed_ns = duration_cast<nanoseconds>(end - start).count();
        latencies.push_back(elapsed_ns);
    }

    BenchmarkResult result = calculateStats(latencies);
    printResultsWithTarget("Modifier State Deactivate", result, 5.0);

    return result;
}

BenchmarkResult benchmarkModifierStateIsActive() {
    std::cout << "\n-------------------------------------------------------------\n";
    std::cout << "Modifier State Query (isActive)\n";
    std::cout << "-------------------------------------------------------------\n";

    ModifierState modState;
    modState.activate(Modifier::Type_Mod9);

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        modState.isActive(Modifier::Type_Mod9);
    }

    // Benchmark isActive()
    std::vector<double> latencies;
    latencies.reserve(BENCHMARK_ITERATIONS);

    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        auto start = high_resolution_clock::now();
        volatile bool active = modState.isActive(Modifier::Type_Mod9);
        auto end = high_resolution_clock::now();

        (void)active;  // Prevent optimization

        double elapsed_ns = duration_cast<nanoseconds>(end - start).count();
        latencies.push_back(elapsed_ns);
    }

    BenchmarkResult result = calculateStats(latencies);
    printResultsWithTarget("Modifier State Query", result, 5.0);

    return result;
}

//=============================================================================
// Benchmark 3: Multiple Concurrent Modal Modifiers
//=============================================================================

BenchmarkResult benchmarkMultipleModalModifiers() {
    std::cout << "\n=============================================================\n";
    std::cout << "Benchmark 3: Multiple Concurrent Modal Modifiers\n";
    std::cout << "=============================================================\n";

    ModifierState modState;

    std::cout << "\nConfiguration:\n";
    std::cout << "  Test: Activate mod0, mod5, mod9, mod15, mod19 concurrently\n";
    std::cout << "  Warmup iterations:   " << WARMUP_ITERATIONS << "\n";
    std::cout << "  Benchmark iterations:" << BENCHMARK_ITERATIONS << "\n";

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        modState.activate(Modifier::Type_Mod0);
        modState.activate(Modifier::Type_Mod5);
        modState.activate(Modifier::Type_Mod9);
        modState.activate(Modifier::Type_Mod15);
        modState.activate(Modifier::Type_Mod19);
        modState.clear();
    }

    // Benchmark: Activate 5 modifiers
    std::vector<double> latencies;
    latencies.reserve(BENCHMARK_ITERATIONS);

    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        modState.clear();

        auto start = high_resolution_clock::now();
        modState.activate(Modifier::Type_Mod0);
        modState.activate(Modifier::Type_Mod5);
        modState.activate(Modifier::Type_Mod9);
        modState.activate(Modifier::Type_Mod15);
        modState.activate(Modifier::Type_Mod19);
        auto end = high_resolution_clock::now();

        double elapsed_ns = duration_cast<nanoseconds>(end - start).count();
        latencies.push_back(elapsed_ns);
    }

    BenchmarkResult result = calculateStats(latencies);
    printResultsWithTarget("Activate 5 Modal Modifiers", result, 25.0);  // 5 * 5μs

    return result;
}

//=============================================================================
// Benchmark 4: Hold-to-Modifier Activation (with sleep simulation)
//=============================================================================

BenchmarkResult benchmarkHoldToModifierActivation() {
    std::cout << "\n=============================================================\n";
    std::cout << "Benchmark 4: Hold-to-Modifier Activation (after threshold)\n";
    std::cout << "=============================================================\n";

    ModifierKeyHandler handler(200);  // 200ms threshold
    handler.registerNumberModifier(0x0002, HardwareModifier::LSHIFT);

    std::cout << "\nConfiguration:\n";
    std::cout << "  Test: PRESS → sleep(210ms) → PRESS (check threshold)\n";
    std::cout << "  This measures threshold check + ACTIVATE action\n";
    std::cout << "  Warmup iterations:   10 (slow due to sleep)\n";
    std::cout << "  Benchmark iterations:1000 (slow due to sleep)\n";

    // Warmup
    for (int i = 0; i < 10; i++) {
        handler.processNumberKey(0x0002, EventType::PRESS);
        std::this_thread::sleep_for(milliseconds(210));
        handler.processNumberKey(0x0002, EventType::PRESS);
        handler.processNumberKey(0x0002, EventType::RELEASE);
    }

    // Benchmark: Threshold exceeded detection
    std::vector<double> latencies;
    latencies.reserve(1000);

    for (int i = 0; i < 1000; i++) {
        // Initial PRESS (starts timer)
        handler.processNumberKey(0x0002, EventType::PRESS);

        // Wait for threshold
        std::this_thread::sleep_for(milliseconds(210));

        // Second PRESS (should detect ACTIVATE)
        auto start = high_resolution_clock::now();
        NumberKeyResult result = handler.processNumberKey(0x0002, EventType::PRESS);
        auto end = high_resolution_clock::now();

        double elapsed_ns = duration_cast<nanoseconds>(end - start).count();
        latencies.push_back(elapsed_ns);

        // Reset
        handler.processNumberKey(0x0002, EventType::RELEASE);
    }

    BenchmarkResult result = calculateStats(latencies);
    printResultsWithTarget("Hold Detection After Threshold", result, 10.0);

    return result;
}

//=============================================================================
// Benchmark 5: Full Pipeline Simulation (ModifierKeyHandler + ModifierState)
//=============================================================================

BenchmarkResult benchmarkFullPipeline() {
    std::cout << "\n=============================================================\n";
    std::cout << "Benchmark 5: Full Pipeline (Handler + State Update)\n";
    std::cout << "=============================================================\n";

    ModifierKeyHandler handler(200);
    handler.registerNumberModifier(0x0002, HardwareModifier::LSHIFT);
    ModifierState modState;

    std::cout << "\nConfiguration:\n";
    std::cout << "  Test: processNumberKey → activate/deactivate ModifierState\n";
    std::cout << "  Simulates real event processing flow\n";
    std::cout << "  Warmup iterations:   " << WARMUP_ITERATIONS << "\n";
    std::cout << "  Benchmark iterations:" << BENCHMARK_ITERATIONS << "\n";

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        auto result = handler.processNumberKey(0x0002, EventType::PRESS);
        if (result.action == ProcessingAction::ACTIVATE_MODIFIER) {
            // In real code, would activate Modifier::Type_Mod0 or similar
            modState.activate(Modifier::Type_Mod0);
        }
        handler.processNumberKey(0x0002, EventType::RELEASE);
        modState.deactivate(Modifier::Type_Mod0);
    }

    // Benchmark full pipeline
    std::vector<double> latencies;
    latencies.reserve(BENCHMARK_ITERATIONS);

    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        auto start = high_resolution_clock::now();

        // Process key event
        auto result = handler.processNumberKey(0x0002, EventType::PRESS);

        // Update modifier state based on result
        if (result.action == ProcessingAction::ACTIVATE_MODIFIER) {
            modState.activate(Modifier::Type_Mod0);
        }

        auto end = high_resolution_clock::now();

        double elapsed_ns = duration_cast<nanoseconds>(end - start).count();
        latencies.push_back(elapsed_ns);

        // Reset
        handler.processNumberKey(0x0002, EventType::RELEASE);
        modState.deactivate(Modifier::Type_Mod0);
    }

    BenchmarkResult result = calculateStats(latencies);
    printResultsWithTarget("Full Pipeline", result, 1000.0);  // 1ms = 1000μs

    return result;
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char** argv) {
    std::cout << "=============================================================\n";
    std::cout << "Modal Modifier Performance Benchmark\n";
    std::cout << "=============================================================\n";
    std::cout << "\nThis benchmark measures latency of modal modifier components:\n";
    std::cout << "  1. Hold detection (ModifierKeyHandler)\n";
    std::cout << "  2. Modifier state updates (activate/deactivate/isActive)\n";
    std::cout << "  3. Multiple concurrent modal modifiers\n";
    std::cout << "  4. Hold-to-modifier activation (threshold check)\n";
    std::cout << "  5. Full pipeline (handler + state)\n";

    // Disable debug logging for accurate benchmarking
    unsetenv("YAMY_DEBUG_KEYCODE");

    // Run all benchmarks
    std::map<std::string, BenchmarkResult> results;

    results["HoldDetection"] = benchmarkHoldDetection();
    results["StateActivate"] = benchmarkModifierStateActivate();
    results["StateDeactivate"] = benchmarkModifierStateDeactivate();
    results["StateQuery"] = benchmarkModifierStateIsActive();
    results["MultipleModifiers"] = benchmarkMultipleModalModifiers();
    results["HoldWithThreshold"] = benchmarkHoldToModifierActivation();
    results["FullPipeline"] = benchmarkFullPipeline();

    // Output CSV
    outputCSV("benchmarks/results/modal_modifier_latency.csv", results);

    // Summary
    std::cout << "\n=============================================================\n";
    std::cout << "Summary\n";
    std::cout << "=============================================================\n";

    std::cout << "\nPerformance Requirements:\n";

    bool hold_pass = results["HoldDetection"].p99_ns < 10000.0;
    bool state_activate_pass = results["StateActivate"].p99_ns < 5000.0;
    bool state_deactivate_pass = results["StateDeactivate"].p99_ns < 5000.0;
    bool pipeline_pass = results["FullPipeline"].p99_ns < 1000000.0;

    std::cout << "  [" << (hold_pass ? "✓" : "✗")
              << "] Hold detection P99 < 10μs ("
              << results["HoldDetection"].p99_ns / 1000.0 << "μs)\n";

    std::cout << "  [" << (state_activate_pass ? "✓" : "✗")
              << "] State activate P99 < 5μs ("
              << results["StateActivate"].p99_ns / 1000.0 << "μs)\n";

    std::cout << "  [" << (state_deactivate_pass ? "✓" : "✗")
              << "] State deactivate P99 < 5μs ("
              << results["StateDeactivate"].p99_ns / 1000.0 << "μs)\n";

    std::cout << "  [" << (pipeline_pass ? "✓" : "✗")
              << "] Full pipeline P99 < 1ms ("
              << results["FullPipeline"].p99_ns / 1000000.0 << "ms)\n";

    bool all_pass = hold_pass && state_activate_pass && state_deactivate_pass && pipeline_pass;

    std::cout << "\n" << (all_pass ? "✓ ALL REQUIREMENTS MET" : "✗ SOME REQUIREMENTS FAILED") << "\n\n";

    return all_pass ? 0 : 1;
}
