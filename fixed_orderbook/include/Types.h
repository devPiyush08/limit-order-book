#pragma once
#include <cstdint>
#include <string>
#include <chrono>

// ─── Primitive aliases ────────────────────────────────────────────────────────
using OrderId   = uint64_t;
using Price     = int64_t;   // price in ticks (e.g. cents × 100)
using Quantity  = uint64_t;
using Timestamp = uint64_t;  // nanoseconds since epoch

// ─── Side ────────────────────────────────────────────────────────────────────
enum class Side : uint8_t { BUY = 0, SELL = 1 };

inline const char* sideStr(Side s) { return s == Side::BUY ? "BUY" : "SELL"; }

// ─── Order type ──────────────────────────────────────────────────────────────
enum class OrderType : uint8_t { LIMIT = 0, MARKET = 1, IOC = 2, FOK = 3 };

// ─── Order status ────────────────────────────────────────────────────────────
enum class OrderStatus : uint8_t {
    NEW, PARTIAL, FILLED, CANCELLED, REJECTED
};

// ─── Single order ────────────────────────────────────────────────────────────
struct Order {
    OrderId    id;
    Price      price;
    Quantity   qty;
    Quantity   filledQty { 0 };
    Side       side;
    OrderType  type      { OrderType::LIMIT };
    OrderStatus status   { OrderStatus::NEW };
    Timestamp  timestamp;
    std::string symbol;

    Quantity remainingQty() const { return qty - filledQty; }
    bool     isFilled()     const { return filledQty >= qty; }
};

// ─── Trade (match result) ─────────────────────────────────────────────────────
struct Trade {
    OrderId   buyOrderId;
    OrderId   sellOrderId;
    Price     price;
    Quantity  qty;
    Timestamp timestamp;
    std::string symbol;
};

// ─── Inline nano-clock ───────────────────────────────────────────────────────
inline Timestamp nowNs() {
    return static_cast<Timestamp>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count()
    );
}
