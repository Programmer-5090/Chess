#ifndef PERF_PROFILER_H
#define PERF_PROFILER_H

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <atomic>
#include "logger.h"

// Simple performance profiler that accumulates named timer durations (microseconds)
// Usage: call g_profiler.startTimer("name"); ... g_profiler.endTimer("name");
// At the end call g_profiler.report();

// Simple performance profiler
// Contract:
// - call PerformanceProfiler::startTimer("name") to begin timing an operation
// - call PerformanceProfiler::endTimer("name") to end and log elapsed time

class PerformanceProfiler {
private:
    struct Frame {
        std::string name;
        std::chrono::high_resolution_clock::time_point start;
        long long child_us = 0; // accumulated inclusive time of child timers
        bool is_root = false;
    };

    // Stack of active timers (LIFO)
    std::vector<Frame> stack;

    // Aggregated totals
    std::unordered_map<std::string, long long> inclusive_us; // total inclusive time per name
    std::unordered_map<std::string, long long> exclusive_us; // total exclusive (self) time per name
    // parent -> (child -> inclusive_us)
    std::unordered_map<std::string, std::unordered_map<std::string, long long>> child_inclusive_us;
    // parent -> (child -> count)
    std::unordered_map<std::string, std::unordered_map<std::string, long long>> child_counts;
    // Totals for timers that were started as root (stack depth 0)
    std::unordered_map<std::string, long long> root_inclusive_us;
    std::unordered_map<std::string, long long> root_counts;
    std::unordered_map<std::string, long long> counts;
    bool verbose = false; // when true, emit per-call timing to the logger

public:
    void startTimer(const std::string& operation);
    void endTimer(const std::string& operation);
    // Enable or disable the profiler. When disabled, startTimer/endTimer are no-ops.
    void setEnabled(bool e);
    bool isEnabled() const;
    // Log aggregated report via the project's Logger (INFO level)
    void report() const;
    // Enable/disable per-call logging
    void setVerbose(bool v) { verbose = v; }
    bool isVerbose() const { return verbose; }

    // (no-op) additional declarations removed

    // Detailed item for reporting
    struct DetailedItem {
        std::string name;
        long long inclusive_us = 0;
        long long exclusive_us = 0;
        long long calls = 0;
        // Totals when this timer was started as a root (stack depth 0)
        long long root_inclusive_us = 0;
        long long root_calls = 0;
    };

    // Return sorted detailed items (by inclusive time desc)
    std::vector<DetailedItem> getDetailedItems() const;

    // Return top child contributors for a given parent name: vector of (childName, inclusive_us)
    struct ChildItem { std::string name; long long inclusive_us; long long calls; };
    std::vector<ChildItem> getChildItemsDetailed(const std::string& parent) const;
    // Return totals for timers that were started at stack depth 0
    std::vector<std::pair<std::string, long long>> getRootItems() const;

    // Backwards-compatible: return inclusive sorted pairs
    std::vector<std::pair<std::string, long long>> getSortedItems() const;
};

// Global profiler instance
extern PerformanceProfiler g_profiler;
// Global atomic flag controlling whether profiler timers are active
extern std::atomic<bool> g_profiler_enabled;

// RAII helper for scoping measurements.
// Usage: { ScopedTimer t("my op"); /* work */ }
struct ScopedTimer {
    std::string name;
    explicit ScopedTimer(const std::string& n) : name(n) {
        // Start a named timer on construction
        g_profiler.startTimer(name);
    }

    ~ScopedTimer() {
        // End the named timer on destruction (accumulates into profiler)
        g_profiler.endTimer(name);
    }
};

#endif // PERF_PROFILER_H