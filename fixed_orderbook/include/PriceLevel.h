#pragma once
#include "Types.h"
#include <list>
#include <unordered_map>
#include <memory>

// ─── PriceLevel ───────────────────────────────────────────────────────────────
// Each price point holds a FIFO queue of orders.
// All operations are O(1).
class PriceLevel {
public:
    using OrderList = std::list<std::shared_ptr<Order>>;
    using Iterator  = OrderList::iterator;

    Price    price;
    Quantity totalQty { 0 };

    explicit PriceLevel(Price p) : price(p) {}

    // Add order to back of queue – O(1)
    Iterator addOrder(std::shared_ptr<Order> order) {
        totalQty += order->remainingQty();
        orders_.push_back(order);
        return std::prev(orders_.end());
    }

    // Remove order by iterator – O(1)
    void removeOrder(Iterator it) {
        totalQty -= (*it)->remainingQty();
        orders_.erase(it);
    }

    // Reduce qty (partial fill) – O(1)
    void reduceQty(Quantity qty) {
        if (totalQty >= qty) totalQty -= qty;
        else                 totalQty  = 0;
    }

    bool      empty()      const { return orders_.empty(); }
    OrderList& orders()          { return orders_; }

private:
    OrderList orders_;
};
