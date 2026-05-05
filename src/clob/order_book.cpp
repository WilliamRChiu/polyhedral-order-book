#include <cassert>
#include <optional>
#include <pob/clob/order_book.hpp>
#include <vector>

#include "pob/clob/types.hpp"

namespace pob::clob {
namespace {
// explicit cast required as quantity is a strong enum over int64_t
constexpr std::int64_t raw(Quantity q) {
    return static_cast<std::int64_t>(q);
}
constexpr std::int64_t raw(Price p) {
    return static_cast<std::int64_t>(p);
}
}  // namespace

std::optional<Price> OrderBook::best_bid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}
std::optional<Price> OrderBook::best_ask() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

std::vector<Fill> OrderBook::add_limit(Order order) {
    std::vector<Fill> fills;
    std::int64_t remaining = raw(order.quantity);
    if (order.side == Side::Bid) {
        // Bids get matched to asks while best ask price <= our bid price
        while (remaining > 0 && !asks_.empty() && raw(asks_.begin()->first) <= raw(order.price)) {
            auto priceLevel = asks_.begin();
            std::list<Order>& queue = priceLevel->second;
            Order& restingTopAsk = queue.front();
            const std::int64_t filled = std::min(remaining, raw(restingTopAsk.quantity));
            remaining -= filled;
            fills.push_back(Fill{order.id, restingTopAsk.id, restingTopAsk.price, Quantity{filled}}
            );
            const std::int64_t restingAmountLeft = raw(restingTopAsk.quantity) - filled;
            if (restingAmountLeft == 0) {
                queue.pop_front();
                if (queue.empty()) {
                    asks_.erase(priceLevel);
                }
            } else {
                restingTopAsk.quantity = static_cast<Quantity>(restingAmountLeft);
            }
        }
        if (remaining > 0) {
            order.quantity = static_cast<Quantity>(remaining);
            bids_[order.price].push_back(order);
        }
    } else {
        while (remaining > 0 && !bids_.empty() && raw(bids_.begin()->first) >= raw(order.price)) {
            auto priceLevel = bids_.begin();
            std::list<Order>& queue = priceLevel->second;
            Order& restingTopAsk = queue.front();
            const std::int64_t filled = std::min(remaining, raw(restingTopAsk.quantity));
            remaining -= filled;
            const std::int64_t restingAmountLeft = raw(restingTopAsk.quantity) - filled;
            fills.push_back(Fill{order.id, restingTopAsk.id, restingTopAsk.price, Quantity{filled}}
            );
            if (restingAmountLeft == 0) {
                queue.pop_front();
                if (queue.empty()) {
                    bids_.erase(priceLevel);
                }
            } else {
                restingTopAsk.quantity = static_cast<Quantity>(restingAmountLeft);
            }
        }
        if (remaining > 0) {
            order.quantity = static_cast<Quantity>(remaining);
            asks_[order.price].push_back(order);
        }
    }
    assert(
        bids_.empty() || asks_.empty() || bids_.begin()->first < asks_.begin()->first
    );  // either one side is empty or best bid is less than best ask
    return fills;
}
}  // namespace pob::clob
