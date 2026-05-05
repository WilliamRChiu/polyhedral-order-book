#pragma once
#include <map>
#include <list>
#include <optional>
#include <pob/clob/types.hpp>
#include <vector>

namespace pob::clob {
    class OrderBook {
        public:
            //query methods
            std::optional<Price> best_bid() const;
            std::optional<Price> best_ask() const;
            //modification methods
            std::vector<Fill> add_limit(Order order);
        private:
            //storage
            std::map<Price, std::list<Order>, std::greater<>> bids_; // Bids sorted by price (highest first)
            std::map<Price, std::list<Order>, std::less<>> asks_; // Asks sorted by price (lowest first)
    };
}
