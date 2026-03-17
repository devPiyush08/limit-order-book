#pragma once
#include "Types.h"
#include <vector>
#include <algorithm>
#include <atomic>
#include <sstream>
#include <mutex>

class LatencyTracker {
public:
    void record(uint64_t latencyNs) {
        std::lock_guard<std::mutex> lk(mu_);
        samples_.push_back(latencyNs);
        total_ += latencyNs;
    }

    void reset() {
        std::lock_guard<std::mutex> lk(mu_);
        samples_.clear();
        total_ = 0;
    }

    std::string report() const {
        std::lock_guard<std::mutex> lk(mu_);
        if (samples_.empty()) return "No samples";

        std::vector<uint64_t> s = samples_;
        std::sort(s.begin(), s.end());

        uint64_t avg = total_ / s.size();

        auto pct = [&](double p) -> uint64_t {
            size_t idx = (size_t)(p / 100.0 * s.size());
            if (idx >= s.size()) idx = s.size() - 1;
            return s[idx];
        };

        std::ostringstream oss;
        oss << "  Samples : " << s.size()   << "\n"
            << "  Min     : " << s.front()   << " ns\n"
            << "  Avg     : " << avg         << " ns\n"
            << "  P50     : " << pct(50)     << " ns\n"
            << "  P90     : " << pct(90)     << " ns\n"
            << "  P99     : " << pct(99)     << " ns\n"
            << "  P99.9   : " << pct(99.9)   << " ns\n"
            << "  Max     : " << s.back()    << " ns\n";
        return oss.str();
    }

    size_t count() const {
        std::lock_guard<std::mutex> lk(mu_);
        return samples_.size();
    }

private:
    mutable std::mutex    mu_;
    std::vector<uint64_t> samples_;
    uint64_t              total_ { 0 };
};

class ThroughputCounter {
public:
    void increment(uint64_t n = 1) { count_.fetch_add(n, std::memory_order_relaxed); }
    uint64_t value() const { return count_.load(std::memory_order_relaxed); }
    void     reset()       { count_.store(0, std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> count_;
};
