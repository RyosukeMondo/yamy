#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// metrics.h - Performance metrics collection for YAMY
//
// High-performance metrics collection with minimal overhead (<1% CPU).
// Thread-safe, lock-free recording with periodic stats computation.
//
// Usage:
//   METRICS_RECORD("key_processing", duration_ns);
//   auto stats = PerformanceMetrics::instance().getStats("key_processing");
//

#ifndef _METRICS_H
#define _METRICS_H

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>
#include <functional>

namespace yamy::metrics {

/// Statistics for a single metric
struct MetricStats {
    std::string name;
    uint64_t count = 0;
    double averageNs = 0.0;
    double p50Ns = 0.0;
    double p95Ns = 0.0;
    double p99Ns = 0.0;
    double minNs = 0.0;
    double maxNs = 0.0;
    uint64_t periodStart = 0;  // Unix timestamp ms
    uint64_t periodEnd = 0;    // Unix timestamp ms
};

/// Ring buffer for latency samples (lock-free write)
class LatencyRingBuffer {
public:
    static constexpr size_t BUFFER_SIZE = 4096;  // Power of 2 for fast modulo

    LatencyRingBuffer() : m_writeIndex(0), m_samples{} {}

    /// Record a latency sample (lock-free)
    void record(uint64_t durationNs) {
        size_t index = m_writeIndex.fetch_add(1, std::memory_order_relaxed) % BUFFER_SIZE;
        m_samples[index].store(durationNs, std::memory_order_relaxed);
    }

    /// Get all samples for stats computation (requires external sync)
    std::vector<uint64_t> getSamples() const {
        std::vector<uint64_t> result;
        result.reserve(BUFFER_SIZE);
        for (size_t i = 0; i < BUFFER_SIZE; ++i) {
            uint64_t val = m_samples[i].load(std::memory_order_relaxed);
            if (val > 0) {
                result.push_back(val);
            }
        }
        return result;
    }

    /// Get sample count (approximate)
    uint64_t getCount() const {
        return m_writeIndex.load(std::memory_order_relaxed);
    }

    /// Clear all samples
    void clear() {
        for (size_t i = 0; i < BUFFER_SIZE; ++i) {
            m_samples[i].store(0, std::memory_order_relaxed);
        }
        m_writeIndex.store(0, std::memory_order_relaxed);
    }

private:
    std::atomic<size_t> m_writeIndex;
    std::atomic<uint64_t> m_samples[BUFFER_SIZE];
};

/// RAII timer for automatic latency recording
class ScopedTimer {
public:
    using Callback = std::function<void(uint64_t)>;

    explicit ScopedTimer(Callback callback)
        : m_callback(std::move(callback))
        , m_start(std::chrono::high_resolution_clock::now())
    {}

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start);
        if (m_callback) {
            m_callback(static_cast<uint64_t>(duration.count()));
        }
    }

    // Non-copyable, non-movable
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    ScopedTimer(ScopedTimer&&) = delete;
    ScopedTimer& operator=(ScopedTimer&&) = delete;

private:
    Callback m_callback;
    std::chrono::high_resolution_clock::time_point m_start;
};

/// Main performance metrics collector (singleton)
class PerformanceMetrics {
public:
    static PerformanceMetrics& instance() {
        static PerformanceMetrics metrics;
        return metrics;
    }

    /// Record a latency sample for a named operation
    void recordLatency(const std::string& operation, uint64_t durationNs) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& buffer = m_buffers[operation];
        buffer.record(durationNs);
    }

    /// Get statistics for a specific operation
    MetricStats getStats(const std::string& operation) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_buffers.find(operation);
        if (it == m_buffers.end()) {
            return MetricStats{operation};
        }
        return computeStats(operation, it->second);
    }

    /// Get statistics for all operations
    std::vector<MetricStats> getAllStats() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<MetricStats> result;
        result.reserve(m_buffers.size());
        for (auto& [name, buffer] : m_buffers) {
            result.push_back(computeStats(name, buffer));
        }
        return result;
    }

    /// Get statistics as formatted string (for IPC/logging)
    std::string getStatsString();

    /// Reset all metrics
    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [name, buffer] : m_buffers) {
            buffer.clear();
        }
        m_lastReportTime = std::chrono::steady_clock::now();
    }

    /// Start periodic logging (every intervalSec seconds)
    void startPeriodicLogging(int intervalSec = 60);

    /// Stop periodic logging
    void stopPeriodicLogging();

    /// Check if periodic logging is active
    bool isLoggingActive() const { return m_loggingActive.load(); }

    /// Create a scoped timer that records to this metric
    ScopedTimer scopedTimer(const std::string& operation) {
        return ScopedTimer([this, operation](uint64_t ns) {
            this->recordLatency(operation, ns);
        });
    }

private:
    PerformanceMetrics();
    ~PerformanceMetrics();

    PerformanceMetrics(const PerformanceMetrics&) = delete;
    PerformanceMetrics& operator=(const PerformanceMetrics&) = delete;

    MetricStats computeStats(const std::string& name, LatencyRingBuffer& buffer);
    void loggingThread();

    std::mutex m_mutex;
    std::unordered_map<std::string, LatencyRingBuffer> m_buffers;
    std::chrono::steady_clock::time_point m_lastReportTime;

    // Periodic logging
    std::atomic<bool> m_loggingActive;
    std::atomic<bool> m_stopLogging;
    std::thread m_loggingThread;
    int m_loggingIntervalSec;
};

// Convenience macros for metrics recording
#define METRICS_RECORD(operation, duration_ns) \
    yamy::metrics::PerformanceMetrics::instance().recordLatency(operation, duration_ns)

#define METRICS_SCOPED_TIMER(operation) \
    auto _metrics_timer_##__LINE__ = yamy::metrics::PerformanceMetrics::instance().scopedTimer(operation)

// Operation names (constants for consistency)
namespace Operations {
    constexpr const char* KEY_PROCESSING = "key_processing";
    constexpr const char* HOOK_CALLBACK = "hook_callback";
    constexpr const char* INPUT_INJECTION = "input_injection";
    constexpr const char* KEYCODE_LOOKUP = "keycode_lookup";
    constexpr const char* WINDOW_QUERY = "window_query";
}

} // namespace yamy::metrics

#endif // _METRICS_H
