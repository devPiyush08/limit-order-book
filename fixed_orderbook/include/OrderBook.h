#pragma once
#include "Types.h"
#include "PriceLevel.h"

#include <map>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>

class OrderBook {
public:
    using TradeCallback  = std::function<void(const Trade&)>;
    using OrderCallback  = std::function<void(const Order&)>;

    explicit OrderBook(std::string symbol) : symbol_(std::move(symbol)) {}

    void onTrade(TradeCallback cb)        { tradeCallback_  = std::move(cb); }
    void onOrderUpdate(OrderCallback cb)  { orderCallback_  = std::move(cb); }

    OrderId addOrder(Side side, Price price, Quantity qty,
                     OrderType type = OrderType::LIMIT) {
        std::shared_ptr<Order> order = std::make_shared<Order>();
        order->id        = nextId_++;
        order->side      = side;
        order->price     = price;
        order->qty       = qty;
        order->type      = type;
        order->status    = OrderStatus::NEW;
        order->timestamp = nowNs();
        order->symbol    = symbol_;

        if (type == OrderType::MARKET) {
            order->price = (side == Side::BUY)
                ? std::numeric_limits<Price>::max()
                : std::numeric_limits<Price>::min();
        }

        match(order);

        if (!order->isFilled()) {
            if (type == OrderType::IOC || type == OrderType::FOK) {
                order->status = OrderStatus::CANCELLED;
                notify(*order);
            } else {
                insertResting(order);
            }
        }
        totalOrders_++;
        return order->id;
    }

    bool cancelOrder(OrderId id) {
        std::unordered_map<OrderId,Entry>::iterator it = orderIndex_.find(id);
        if (it == orderIndex_.end()) return false;

        Entry& e = it->second;
        std::shared_ptr<Order> order = *(e.listIt);

        if (e.side == Side::BUY) {
            PriceLevel& level = e.bidLevelIt->second;
            level.reduceQty(order->remainingQty());
            level.removeOrder(e.listIt);
            if (level.empty()) bids_.erase(e.bidLevelIt);
        } else {
            PriceLevel& level = e.askLevelIt->second;
            level.reduceQty(order->remainingQty());
            level.removeOrder(e.listIt);
            if (level.empty()) asks_.erase(e.askLevelIt);
        }

        order->status = OrderStatus::CANCELLED;
        notify(*order);
        orderIndex_.erase(it);
        return true;
    }

    OrderId modifyOrder(OrderId id, Price newPrice, Quantity newQty) {
        std::unordered_map<OrderId,Entry>::iterator it = orderIndex_.find(id);
        if (it == orderIndex_.end()) return 0;
        Side side = it->second.side;
        cancelOrder(id);
        return addOrder(side, newPrice, newQty, OrderType::LIMIT);
    }

    Price    bestBid()     const { return bids_.empty() ? 0 : bids_.begin()->first; }
    Price    bestAsk()     const { return asks_.empty() ? 0 : asks_.begin()->first; }
    Price    spread()      const { return bestAsk() - bestBid(); }
    uint64_t totalOrders() const { return totalOrders_; }
    uint64_t totalTrades() const { return totalTrades_; }
    uint64_t totalVolume() const { return totalVolume_; }
    const std::string& symbol()  const { return symbol_; }

    struct Level { Price price; Quantity qty; int orders; };
    std::vector<Level> topBids(int n = 5) const { return topLevels(bids_, n); }
    std::vector<Level> topAsks(int n = 5) const { return topLevels(asks_, n); }

    std::string toString(int depth = 5) const {
        std::ostringstream oss;
        std::vector<Level> askLevels = topAsks(depth);
        std::vector<Level> bidLevels = topBids(depth);

        oss << "\n+======================================+\n";
        oss << "|  ORDER BOOK  [" << std::left << std::setw(20) << symbol_ << "]|\n";
        oss << "+======================================+\n";
        oss << "|  Side   Price       Qty      Orders  |\n";
        oss << "+======================================+\n";

        for (int i = (int)askLevels.size()-1; i >= 0; --i) {
            Level& a = askLevels[i];
            oss << "|  ASK  " << std::right
                << std::setw(9)  << priceToStr(a.price)
                << std::setw(11) << a.qty
                << std::setw(8)  << a.orders << "    |\n";
        }
        oss << "|  -- SPREAD: " << std::setw(6) << priceToStr(spread())
            << " ------------------  |\n";
        for (size_t i = 0; i < bidLevels.size(); i++) {
            Level& b = bidLevels[i];
            oss << "|  BID  " << std::right
                << std::setw(9)  << priceToStr(b.price)
                << std::setw(11) << b.qty
                << std::setw(8)  << b.orders << "    |\n";
        }
        oss << "+======================================+\n";
        oss << "  Orders: " << totalOrders_ << "  Trades: " << totalTrades_
            << "  Volume: " << totalVolume_ << "\n";
        return oss.str();
    }

private:
    using BidMap = std::map<Price, PriceLevel, std::greater<Price> >;
    using AskMap = std::map<Price, PriceLevel, std::less<Price> >;

    struct Entry {
        BidMap::iterator     bidLevelIt;
        AskMap::iterator     askLevelIt;
        PriceLevel::Iterator listIt;
        Side                 side;
    };

    std::string   symbol_;
    BidMap        bids_;
    AskMap        asks_;
    std::unordered_map<OrderId, Entry> orderIndex_;

    uint64_t nextId_      { 1 };
    uint64_t totalOrders_ { 0 };
    uint64_t totalTrades_ { 0 };
    uint64_t totalVolume_ { 0 };

    TradeCallback  tradeCallback_;
    OrderCallback  orderCallback_;

    void match(std::shared_ptr<Order>& aggressor) {
        if (aggressor->side == Side::BUY)
            matchAgainst(aggressor, asks_);
        else
            matchAgainst(aggressor, bids_);
    }

    template<typename BookSide>
    void matchAgainst(std::shared_ptr<Order>& aggressor, BookSide& book) {
        while (!aggressor->isFilled() && !book.empty()) {
            typename BookSide::iterator levelIt = book.begin();
            Price levelPx = levelIt->first;

            bool crosses = (aggressor->side == Side::BUY)
                ? (aggressor->price >= levelPx)
                : (aggressor->price <= levelPx);
            if (!crosses) break;

            PriceLevel& level = levelIt->second;
            PriceLevel::OrderList& orderList = level.orders();

            while (!aggressor->isFilled() && !orderList.empty()) {
                std::shared_ptr<Order>& passive = orderList.front();
                Quantity fillQty = std::min(aggressor->remainingQty(),
                                            passive->remainingQty());

                aggressor->filledQty += fillQty;
                passive->filledQty   += fillQty;
                level.reduceQty(fillQty);
                totalVolume_ += fillQty;
                totalTrades_++;

                Trade trade;
                trade.buyOrderId  = (aggressor->side==Side::BUY)  ? aggressor->id : passive->id;
                trade.sellOrderId = (aggressor->side==Side::SELL)  ? aggressor->id : passive->id;
                trade.price       = levelPx;
                trade.qty         = fillQty;
                trade.timestamp   = nowNs();
                trade.symbol      = symbol_;
                if (tradeCallback_) tradeCallback_(trade);

                if (passive->isFilled()) {
                    passive->status = OrderStatus::FILLED;
                    notify(*passive);
                    orderIndex_.erase(passive->id);
                    orderList.pop_front();
                } else {
                    passive->status = OrderStatus::PARTIAL;
                    notify(*passive);
                }

                aggressor->status = aggressor->isFilled()
                    ? OrderStatus::FILLED : OrderStatus::PARTIAL;
                notify(*aggressor);
            }
            if (level.empty()) book.erase(levelIt);
        }
    }

    void insertResting(std::shared_ptr<Order> order) {
        Entry e;
        e.side = order->side;
        if (order->side == Side::BUY) {
            std::pair<BidMap::iterator,bool> res =
                bids_.insert(std::make_pair(order->price, PriceLevel(order->price)));
            e.bidLevelIt = res.first;
            e.listIt     = res.first->second.addOrder(order);
        } else {
            std::pair<AskMap::iterator,bool> res =
                asks_.insert(std::make_pair(order->price, PriceLevel(order->price)));
            e.askLevelIt = res.first;
            e.listIt     = res.first->second.addOrder(order);
        }
        orderIndex_[order->id] = e;
    }

    void notify(const Order& o) {
        if (orderCallback_) orderCallback_(o);
    }

    template<typename Map>
    std::vector<Level> topLevels(const Map& m, int n) const {
        std::vector<Level> out;
        out.reserve(n);
        for (typename Map::const_iterator it = m.begin(); it != m.end(); ++it) {
            if ((int)out.size() >= n) break;
            Level lv;
            lv.price  = it->first;
            lv.qty    = it->second.totalQty;
            lv.orders = (int)const_cast<PriceLevel&>(it->second).orders().size();
            out.push_back(lv);
        }
        return out;
    }

    static std::string priceToStr(Price p) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << (p / 100.0);
        return oss.str();
    }
};
