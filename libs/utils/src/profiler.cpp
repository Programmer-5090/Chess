#include <chess/utils/profiler.h>
#include <chrono>
#include <vector>
#include <algorithm>
#include <sstream>
#include <chess/utils/logger.h>
#include <chess/utils/profiler.h>
#include <chrono>
#include <vector>
#include <algorithm>
#include <sstream>
#include <chess/utils/logger.h>

PerformanceProfiler g_profiler;
std::atomic<bool> g_profiler_enabled{true};

void PerformanceProfiler::setEnabled(bool e) { g_profiler_enabled.store(e, std::memory_order_relaxed); }

bool PerformanceProfiler::isEnabled() const {
    return g_profiler_enabled.load(std::memory_order_relaxed);
}

void PerformanceProfiler::startTimer(const std::string& operation) {
    if (!g_profiler_enabled.load(std::memory_order_relaxed)) return;
    Frame f;
    f.name = operation;
    f.start = std::chrono::high_resolution_clock::now();
    f.child_us = 0;
    f.is_root = stack.empty();
    stack.push_back(std::move(f));
}

void PerformanceProfiler::endTimer(const std::string& operation) {
    if (!g_profiler_enabled.load(std::memory_order_relaxed)) return;
    auto endTime = std::chrono::high_resolution_clock::now();

    if (stack.empty()) return; // mismatched endTimer

    Frame top = stack.back();
    stack.pop_back();

    if (top.name != operation) {
        std::ostringstream warnoss;
        warnoss << "PerformanceProfiler: timer mismatch. Expected '" << top.name << "' got '" << operation << "'";
        Logger::log(LogLevel::WARN, warnoss.str(), __FILE__, __LINE__);
    }

    long long elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(endTime - top.start).count();

    // inclusive adds entire elapsed time
    inclusive_us[top.name] += elapsed_us;
    // exclusive is elapsed minus time spent in child timers
    long long self_us = elapsed_us - top.child_us;
    if (self_us < 0) self_us = 0; // safety
    exclusive_us[top.name] += self_us;
    counts[top.name] += 1;

    // If this frame was started as a root, record root totals
    if (top.is_root) {
        root_inclusive_us[top.name] += elapsed_us;
        root_counts[top.name] += 1;
    }

    // If there's a parent frame on the stack, add this elapsed to its child_us
    if (!stack.empty()) {
        stack.back().child_us += elapsed_us;
    }

    // Record parent->child inclusive mapping if there was a parent
    if (!stack.empty()) {
        const std::string &parent = stack.back().name;
        child_inclusive_us[parent][top.name] += elapsed_us;
        child_counts[parent][top.name] += 1;
    }

    // Emit per-call timing when verbose
    if (g_profiler.isVerbose()) {
        std::ostringstream oss;
        oss << "[PerformanceProfiler] " << top.name << ": " << (elapsed_us / 1000.0) << " ms (self=" << (self_us/1000.0) << " ms)";
        Logger::log(LogLevel::DEBUG, oss.str(), __FILE__, __LINE__);
    }
}

void PerformanceProfiler::report() const {
    std::ostringstream oss;
    oss << "\n=== Performance Profiler Report ===\n";

    // Build detailed items
    std::vector<DetailedItem> items;
    for (const auto &kv : inclusive_us) {
        DetailedItem it;
        it.name = kv.first;
        it.inclusive_us = kv.second;
        auto exIt = exclusive_us.find(kv.first);
        if (exIt != exclusive_us.end()) it.exclusive_us = exIt->second;
        auto cntIt = counts.find(kv.first);
        if (cntIt != counts.end()) it.calls = cntIt->second;
        items.push_back(it);
    }

    // sort by inclusive time desc
    std::sort(items.begin(), items.end(), [](const DetailedItem &a, const DetailedItem &b) {
        return a.inclusive_us > b.inclusive_us;
    });

    for (const auto &p : items) {
        double incl_ms = p.inclusive_us / 1000.0;
        double excl_ms = p.exclusive_us / 1000.0;
        double avg_ms = p.calls ? (p.inclusive_us / 1000.0 / p.calls) : 0.0;
        oss << p.name << ": incl=" << incl_ms << " ms, excl=" << excl_ms << " ms, calls=" << p.calls << ", avg(incl)=" << avg_ms << " ms\n";
    }

    oss << "=== End Performance Report ===\n\n";

    // Use Logger to write the aggregated report at INFO level so it's included in file logs
    Logger::log(LogLevel::INFO, oss.str(), __FILE__, __LINE__);
}

std::vector<PerformanceProfiler::DetailedItem> PerformanceProfiler::getDetailedItems() const {
    std::vector<DetailedItem> items;
    for (const auto &kv : inclusive_us) {
        DetailedItem it;
        it.name = kv.first;
        it.inclusive_us = kv.second;
        auto exIt = exclusive_us.find(kv.first);
        if (exIt != exclusive_us.end()) it.exclusive_us = exIt->second;
        auto cntIt = counts.find(kv.first);
        if (cntIt != counts.end()) it.calls = cntIt->second;
        items.push_back(it);
    }
    std::sort(items.begin(), items.end(), [](const DetailedItem &a, const DetailedItem &b) {
        return a.inclusive_us > b.inclusive_us;
    });
    return items;
}

std::vector<std::pair<std::string, long long>> PerformanceProfiler::getSortedItems() const {
    std::vector<std::pair<std::string, long long>> items;
    for (const auto& kv : inclusive_us) items.emplace_back(kv.first, kv.second);
    std::sort(items.begin(), items.end(), [](auto &a, auto &b){ return a.second > b.second; });
    return items;
}

std::vector<PerformanceProfiler::ChildItem> PerformanceProfiler::getChildItemsDetailed(const std::string& parent) const {
    std::vector<ChildItem> items;
    auto it = child_inclusive_us.find(parent);
    if (it == child_inclusive_us.end()) return items;
    for (const auto &kv : it->second) {
        ChildItem ci;
        ci.name = kv.first;
        ci.inclusive_us = kv.second;
        auto ccIt = child_counts.find(parent);
        if (ccIt != child_counts.end()) {
            auto cIt2 = ccIt->second.find(kv.first);
            if (cIt2 != ccIt->second.end()) ci.calls = cIt2->second;
        }
        items.push_back(ci);
    }
    std::sort(items.begin(), items.end(), [](const ChildItem &a, const ChildItem &b){ return a.inclusive_us > b.inclusive_us; });
    return items;
}

std::vector<std::pair<std::string, long long>> PerformanceProfiler::getRootItems() const {
    std::vector<std::pair<std::string, long long>> items;
    for (const auto &kv : root_inclusive_us) items.emplace_back(kv.first, kv.second);
    std::sort(items.begin(), items.end(), [](auto &a, auto &b){ return a.second > b.second; });
    return items;
}

void PerformanceProfiler::clear() {
    stack.clear();
    inclusive_us.clear();
    exclusive_us.clear();
    child_inclusive_us.clear();
    child_counts.clear();
    root_inclusive_us.clear();
    root_counts.clear();
    counts.clear();
}
