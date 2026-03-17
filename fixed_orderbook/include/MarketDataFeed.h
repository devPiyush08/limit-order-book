#pragma once
#include "Types.h"
#include <random>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

struct FeedEvent {
    enum Type : uint8_t { NEW_ORDER, CANCEL, MODIFY } type;
    Side      side;
    Price     price;
    Quantity  qty;
    OrderId   targetId { 0 };
    Price     newPrice { 0 };
    Quantity  newQty   { 0 };
};

class MarketDataFeed {
public:
    using EventCallback = std::function<void(const FeedEvent&)>;

    explicit MarketDataFeed(Price midPrice = 10000,
                            double tickSize = 1.0,
                            uint32_t targetOps = 5000)
        : mid_(midPrice), tick_(tickSize), targetOps_(targetOps),
          rng_(std::random_device{}()),
          priceDist_(-20, 20),
          qtySide_(1, 200),
          actionDist_(0, 99)
    {}

    void onEvent(EventCallback cb) { callback_ = std::move(cb); }

    void start() {
        running_ = true;
        thread_  = std::thread([this] { run(); });
    }

    void stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }

    uint64_t eventsEmitted() const { return emitted_.load(); }

private:
    Price      mid_;
    double     tick_;
    uint32_t   targetOps_;
    std::atomic<bool>     running_;
    std::atomic<uint64_t> emitted_;
    std::thread thread_;
    EventCallback callback_;

    std::mt19937_64 rng_;
    std::uniform_int_distribution<int>      priceDist_;
    std::uniform_int_distribution<uint64_t> qtySide_;
    std::uniform_int_distribution<int>      actionDist_;

    std::vector<OrderId> liveIds_;

    void run() {
        using namespace std::chrono;
        const nanoseconds periodNs(1000000000ULL / targetOps_);
        auto next = high_resolution_clock::now();

        while (running_.load()) {
            emit();
            next += periodNs;
            std::this_thread::sleep_until(next);
        }
    }

    void emit() {
        if (!callback_) return;

        int action = actionDist_(rng_);
        FeedEvent ev;

        if (liveIds_.empty() || action < 70) {
            ev.type  = FeedEvent::NEW_ORDER;
            ev.side  = (action % 2 == 0) ? Side::BUY : Side::SELL;
            ev.price = mid_ + static_cast<Price>(priceDist_(rng_));
            ev.qty   = qtySide_(rng_);
        } else if (action < 90 && !liveIds_.empty()) {
            ev.type     = FeedEvent::CANCEL;
            size_t idx  = std::uniform_int_distribution<size_t>(0, liveIds_.size()-1)(rng_);
            ev.targetId = liveIds_[idx];
            liveIds_.erase(liveIds_.begin() + idx);
        } else if (!liveIds_.empty()) {
            ev.type     = FeedEvent::MODIFY;
            size_t idx  = std::uniform_int_distribution<size_t>(0, liveIds_.size()-1)(rng_);
            ev.targetId = liveIds_[idx];
            ev.newPrice = mid_ + static_cast<Price>(priceDist_(rng_));
            ev.newQty   = qtySide_(rng_);
            liveIds_.erase(liveIds_.begin() + idx);
        } else {
            return;
        }

        callback_(ev);
        emitted_++;

        if (emitted_ % 500 == 0) {
            mid_ += static_cast<Price>((actionDist_(rng_) % 3) - 1);
        }
    }
};
