#pragma once
#include <cstddef>
#include <optional>
#include <pob/polyhedral/types.hpp>
#include <unordered_map>

namespace pob::polyhedral{
    class PolyhedralBook{
        public:
            PolyhedralBook(int N){
                this->N_ = N;
            };
            std::vector<Fill> add_order(Order o);
            void cancel(OrderId id);
            std::size_t num_orders() const;
            void seed_price_vector(std::vector<double> y);

        private:
            int N_;
            std::unordered_map<OrderId, Order> orders_;
            std::vector<OrderId> insertion_order_;
            std::optional<std::vector<double>> y_cache_; //could rework later to not be O(M) time complexity
            void rest_(Order&& o); //helper adding order to book without attempting to match

    };
}