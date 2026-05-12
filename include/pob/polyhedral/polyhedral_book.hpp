#pragma once
#include "pob/polyhedral/matching.hpp"
#include <cstddef>
#include <optional>
#include <pob/polyhedral/types.hpp>
#include <unordered_map>
#include <vector>

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
            std::vector<std::vector<double>> assemble_columns_() const; //walk insertion_order_ and return each order's column * remaining
            static double price_of_(const std::vector<double>& column); //fills Fill.price
            double apply_matching_(const MatchingResult& m, const std::vector<std::size_t>& active, OrderId aggressor_id, std::vector<Fill>& fills_out); //return (1-m.x_M) so caller can multiply remaining_M


    };
}