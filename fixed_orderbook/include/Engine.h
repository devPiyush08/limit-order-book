#pragma once
#include "OrderBook.h"
#include "MarketDataFeed.h"
#include "Metrics.h"
#include "SPSCQueue.h"

#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

class Engine {
public:
    struct Config {
        std::string symbol      { "AAPL" };
        Price       midPrice    { 15000 };
        uint32_t    targetOps   { 5000  };
        int         displaySecs { 2     };
        int         runSecs     { 30    };
        bool        verbose     { false };
    };

    explicit Engine(Config cfg)
        : cfg_(std::move(cfg)),
          book_(cfg_.symbol),
          feed_(cfg_.midPrice, 1.0, cfg_.targetOps)
    {
        book_.onTrade([this](const Trade& t) {
            ops_.increment();
            if (cfg_.verbose) {
                std::cout << "[TRADE] " << t.symbol
                          << " px=" << (t.price/100.0)
                          << " qty=" << t.qty
                          << " buy#" << t.buyOrderId
                          << " sell#" << t.sellOrderId << "\n";
            }
        });

        feed_.onEvent([this](const FeedEvent& ev) {
            while (!queue_.push(ev)) { /* spin if full */ }
        });
    }

    void run() {
        using namespace std::chrono;
        auto startTime = high_resolution_clock::now();
        auto nextPrint = startTime + seconds(cfg_.displaySecs);

        feed_.start();

        std::cout << "\n==> Engine started  [" << cfg_.symbol << "]"
                  << "  target=" << cfg_.targetOps << " ops/s"
                  << "  runSecs=" << cfg_.runSecs << "\n\n";

        while (true) {
            auto now = high_resolution_clock::now();

            if (cfg_.runSecs > 0 &&
                duration_cast<seconds>(now - startTime).count() >= cfg_.runSecs)
                break;

            FeedEvent ev;
            while (queue_.pop(ev)) {
                uint64_t t0 = nowNs();
                processEvent(ev);
                uint64_t t1 = nowNs();
                latency_.record(t1 - t0);
                ops_.increment();
            }

            if (now >= nextPrint) {
                printStats((int)duration_cast<seconds>(now - startTime).count());
                nextPrint = now + seconds(cfg_.displaySecs);
            }
        }

        feed_.stop();
        printFinalReport();
    }

private:
    Config         cfg_;
    OrderBook      book_;
    MarketDataFeed feed_;
    LatencyTracker latency_;
    ThroughputCounter ops_;

    SPSCQueue<FeedEvent, 65536> queue_;
    std::vector<OrderId> liveIds_;

    void processEvent(const FeedEvent& ev) {
        switch (ev.type) {
        case FeedEvent::NEW_ORDER: {
            OrderId id = book_.addOrder(ev.side, ev.price, ev.qty);
            if (id) liveIds_.push_back(id);
            break;
        }
        case FeedEvent::CANCEL:
            book_.cancelOrder(ev.targetId);
            removeLiveId(ev.targetId);
            break;
        case FeedEvent::MODIFY:
            book_.modifyOrder(ev.targetId, ev.newPrice, ev.newQty);
            removeLiveId(ev.targetId);
            break;
        }
    }

    void removeLiveId(OrderId id) {
        std::vector<OrderId>::iterator it =
            std::find(liveIds_.begin(), liveIds_.end(), id);
        if (it != liveIds_.end()) liveIds_.erase(it);
    }

    void printStats(int elapsed) {
        uint64_t totalOps = ops_.value();
        double   opsPerSec = (elapsed > 0) ? (double)totalOps / elapsed : 0;

        std::cout << book_.toString();
        std::cout << "\nPERFORMANCE [elapsed=" << elapsed << "s]\n";
        std::cout << "  Total ops   : " << totalOps   << "\n";
        std::cout << "  Throughput  : " << std::fixed << std::setprecision(0)
                  << opsPerSec << " ops/s\n";
        std::cout << "\nLATENCY (ns)\n";
        std::cout << latency_.report();
        std::cout << "-----------------------------------------\n\n";
    }

    void printFinalReport() {
        std::cout << "\n=========================================\n";
        std::cout << "  FINAL REPORT  [" << cfg_.symbol << "]\n";
        std::cout << "=========================================\n";
        std::cout << "  Total orders : " << book_.totalOrders() << "\n";
        std::cout << "  Total trades : " << book_.totalTrades() << "\n";
        std::cout << "  Total volume : " << book_.totalVolume() << "\n";
        std::cout << "  Feed emitted : " << feed_.eventsEmitted() << "\n";
        std::cout << "\nFULL LATENCY HISTOGRAM\n";
        std::cout << latency_.report();
        std::cout << "=========================================\n";
    }
};
