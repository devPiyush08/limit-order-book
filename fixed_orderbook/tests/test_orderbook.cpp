#include "../include/OrderBook.h"
#include <cassert>
#include <iostream>
#include <iomanip>

// ─── Simple test harness ──────────────────────────────────────────────────────
int passed = 0, failed = 0;

#define TEST(name) void test_##name()
#define RUN(name) do { \
    std::cout << "  [TEST] " << std::left << std::setw(40) << #name; \
    try { test_##name(); std::cout << "PASS\n"; ++passed; } \
    catch (std::exception& e) { std::cout << "FAIL: " << e.what() << "\n"; ++failed; } \
} while(0)

#define ASSERT(cond) if(!(cond)) throw std::runtime_error("Assert failed: " #cond)
#define ASSERT_EQ(a,b) if((a)!=(b)) { \
    std::ostringstream _o; _o << "Expected " << (b) << " got " << (a); \
    throw std::runtime_error(_o.str()); }

// ─── Tests ────────────────────────────────────────────────────────────────────

TEST(basic_bid_ask) {
    OrderBook book("TEST");
    book.addOrder(Side::BUY,  10000, 100);
    book.addOrder(Side::SELL, 10100, 100);
    ASSERT_EQ(book.bestBid(), 10000);
    ASSERT_EQ(book.bestAsk(), 10100);
    ASSERT_EQ(book.spread(),  100);
}

TEST(full_match) {
    OrderBook book("TEST");
    int trades = 0;
    book.onTrade([&](const Trade& t) {
        ++trades;
        ASSERT_EQ(t.qty, 50);
        ASSERT_EQ(t.price, 10000);
    });
    book.addOrder(Side::SELL, 10000, 50);
    book.addOrder(Side::BUY,  10000, 50);
    ASSERT_EQ(trades, 1);
    ASSERT_EQ(book.bestBid(), 0);  // book is clear
    ASSERT_EQ(book.bestAsk(), 0);
}

TEST(partial_match) {
    OrderBook book("TEST");
    int trades = 0;
    book.onTrade([&](const Trade&) { ++trades; });
    book.addOrder(Side::SELL, 10000, 100);
    book.addOrder(Side::BUY,  10000, 60);
    ASSERT_EQ(trades, 1);
    ASSERT(book.bestAsk() == 10000);  // 40 qty still resting
}

TEST(cancel_order) {
    OrderBook book("TEST");
    auto id = book.addOrder(Side::BUY, 9900, 100);
    ASSERT_EQ(book.bestBid(), 9900);
    bool ok = book.cancelOrder(id);
    ASSERT(ok);
    ASSERT_EQ(book.bestBid(), 0);
}

TEST(price_priority) {
    // Better-priced orders should trade first
    OrderBook book("TEST");
    std::vector<Price> tradePrices;
    book.onTrade([&](const Trade& t) { tradePrices.push_back(t.price); });

    book.addOrder(Side::SELL, 10100, 50);
    book.addOrder(Side::SELL, 10050, 50);  // better ask – should fill first
    book.addOrder(Side::BUY,  10200, 100); // aggressive buyer

    ASSERT_EQ(tradePrices.size(), (size_t)2);
    ASSERT_EQ(tradePrices[0], 10050);  // lowest ask first
    ASSERT_EQ(tradePrices[1], 10100);
}

TEST(time_priority_fifo) {
    OrderBook book("TEST");
    std::vector<OrderId> filledIds;
    book.onOrderUpdate([&](const Order& o) {
        if (o.status == OrderStatus::FILLED) filledIds.push_back(o.id);
    });

    auto id1 = book.addOrder(Side::SELL, 10000, 50);
    auto id2 = book.addOrder(Side::SELL, 10000, 50);  // same price, later
    book.addOrder(Side::BUY, 10000, 50);              // fills one

    ASSERT(!filledIds.empty());
    ASSERT_EQ(filledIds[0], id1);   // first-in, first-filled
    (void)id2;
}

TEST(ioc_order) {
    OrderBook book("TEST");
    book.addOrder(Side::SELL, 10100, 50);
    // IOC at 10050 can't cross – should be cancelled, not resting
    book.addOrder(Side::BUY, 10050, 100, OrderType::IOC);
    ASSERT_EQ(book.bestBid(), 0);   // IOC shouldn't rest
}

TEST(market_order) {
    OrderBook book("TEST");
    int trades = 0;
    book.onTrade([&](const Trade&) { ++trades; });
    book.addOrder(Side::SELL, 10000, 100);
    book.addOrder(Side::BUY,  0, 100, OrderType::MARKET);
    ASSERT_EQ(trades, 1);
}

TEST(multi_level_exhaustion) {
    OrderBook book("TEST");
    int trades = 0;
    Quantity filled = 0;
    book.onTrade([&](const Trade& t) { ++trades; filled += t.qty; });

    book.addOrder(Side::SELL, 10000, 30);
    book.addOrder(Side::SELL, 10010, 30);
    book.addOrder(Side::SELL, 10020, 30);
    book.addOrder(Side::BUY,  10050, 90);  // should sweep all 3 levels

    ASSERT_EQ(trades, 3);
    ASSERT_EQ(filled, 90);
    ASSERT_EQ(book.bestAsk(), 0);
}

TEST(throughput_5k) {
    // Verify 5000 ops/sec capability (just a speed check, no assertions on time)
    OrderBook book("BENCH");
    const int N = 10000;
    auto t0 = nowNs();
    for (int i = 0; i < N; ++i) {
        Price p = 10000 + (i % 40 - 20);
        Side  s = (i % 2 == 0) ? Side::BUY : Side::SELL;
        book.addOrder(s, p, 10 + (i % 90));
    }
    auto t1 = nowNs();
    double secs = (t1 - t0) / 1e9;
    double ops  = N / secs;
    std::cout << " (" << (uint64_t)ops << " ops/s) ";
    ASSERT(ops > 5000);  // should easily exceed 5k
}

// ─── Main ──────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "\n╔══════════════════════════════════════════╗\n";
    std::cout << "║     ORDER BOOK TEST SUITE                ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n\n";

    RUN(basic_bid_ask);
    RUN(full_match);
    RUN(partial_match);
    RUN(cancel_order);
    RUN(price_priority);
    RUN(time_priority_fifo);
    RUN(ioc_order);
    RUN(market_order);
    RUN(multi_level_exhaustion);
    RUN(throughput_5k);

    std::cout << "\n──────────────────────────────────────────\n";
    std::cout << "  Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "──────────────────────────────────────────\n\n";

    return (failed > 0) ? 1 : 0;
}
