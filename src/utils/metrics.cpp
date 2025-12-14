//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// metrics.cpp - Performance metrics collection implementation
//

#include "metrics.h"
#include "logger.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace yamy::metrics {

PerformanceMetrics::PerformanceMetrics()
    : m_lastReportTime(std::chrono::steady_clock::now())
    , m_loggingActive(false)
    , m_stopLogging(false)
    , m_loggingIntervalSec(60)
{
}

PerformanceMetrics::~PerformanceMetrics()
{
    stopPeriodicLogging();
}

MetricStats PerformanceMetrics::computeStats(const std::string& name, LatencyRingBuffer& buffer)
{
    MetricStats stats;
    stats.name = name;

    auto samples = buffer.getSamples();
    if (samples.empty()) {
        return stats;
    }

    // Sort for percentile computation
    std::sort(samples.begin(), samples.end());

    stats.count = buffer.getCount();
    stats.minNs = static_cast<double>(samples.front());
    stats.maxNs = static_cast<double>(samples.back());

    // Compute average
    double sum = 0.0;
    for (auto val : samples) {
        sum += static_cast<double>(val);
    }
    stats.averageNs = sum / static_cast<double>(samples.size());

    // Compute percentiles
    auto percentile = [&samples](double p) -> double {
        if (samples.empty()) return 0.0;
        double rank = p * static_cast<double>(samples.size() - 1);
        size_t lower = static_cast<size_t>(std::floor(rank));
        size_t upper = static_cast<size_t>(std::ceil(rank));
        if (lower == upper) {
            return static_cast<double>(samples[lower]);
        }
        double fraction = rank - static_cast<double>(lower);
        return static_cast<double>(samples[lower]) * (1.0 - fraction) +
               static_cast<double>(samples[upper]) * fraction;
    };

    stats.p50Ns = percentile(0.50);
    stats.p95Ns = percentile(0.95);
    stats.p99Ns = percentile(0.99);

    // Time period
    auto now = std::chrono::system_clock::now();
    stats.periodEnd = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count());
    stats.periodStart = stats.periodEnd - 60000; // Approximate, last 60s

    return stats;
}

std::string PerformanceMetrics::getStatsString()
{
    auto allStats = getAllStats();

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "=== Performance Metrics ===\n";

    if (allStats.empty()) {
        oss << "No metrics collected yet.\n";
        return oss.str();
    }

    for (const auto& stats : allStats) {
        oss << "\n[" << stats.name << "]\n";
        oss << "  Count:   " << stats.count << "\n";
        oss << "  Average: " << (stats.averageNs / 1000.0) << " us\n";
        oss << "  P50:     " << (stats.p50Ns / 1000.0) << " us\n";
        oss << "  P95:     " << (stats.p95Ns / 1000.0) << " us\n";
        oss << "  P99:     " << (stats.p99Ns / 1000.0) << " us\n";
        oss << "  Min:     " << (stats.minNs / 1000.0) << " us\n";
        oss << "  Max:     " << (stats.maxNs / 1000.0) << " us\n";
    }

    return oss.str();
}

void PerformanceMetrics::startPeriodicLogging(int intervalSec)
{
    if (m_loggingActive.exchange(true)) {
        return; // Already running
    }

    m_loggingIntervalSec = intervalSec;
    m_stopLogging = false;
    m_loggingThread = std::thread(&PerformanceMetrics::loggingThread, this);
}

void PerformanceMetrics::stopPeriodicLogging()
{
    if (!m_loggingActive.load()) {
        return;
    }

    m_stopLogging = true;
    if (m_loggingThread.joinable()) {
        m_loggingThread.join();
    }
    m_loggingActive = false;
}

void PerformanceMetrics::loggingThread()
{
    while (!m_stopLogging) {
        // Sleep in small increments to allow quick shutdown
        for (int i = 0; i < m_loggingIntervalSec * 10 && !m_stopLogging; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (m_stopLogging) break;

        // Log current stats
        auto allStats = getAllStats();
        if (allStats.empty()) {
            continue;
        }

        for (const auto& stats : allStats) {
            if (stats.count == 0) continue;

            LOG_INFO("[metrics] {}: count={} avg={:.2f}us p50={:.2f}us p95={:.2f}us p99={:.2f}us",
                     stats.name,
                     static_cast<unsigned long>(stats.count),
                     stats.averageNs / 1000.0,
                     stats.p50Ns / 1000.0,
                     stats.p95Ns / 1000.0,
                     stats.p99Ns / 1000.0);
        }

        // Reset for next period
        reset();
    }
}

} // namespace yamy::metrics
