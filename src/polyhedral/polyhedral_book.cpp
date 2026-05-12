#include "lp_data/HConst.h"
#include "pob/polyhedral/matching.hpp"
#include "pob/polyhedral/pricing.hpp"
#include "pob/polyhedral/types.hpp"
#include <cmath>
#include <cstddef>
#include <pob/polyhedral/polyhedral_book.hpp>
#include <cassert>
#include <pob/polyhedral/predicates.hpp>
#include <utility>
#include <vector>

namespace pob::polyhedral {

    std::size_t PolyhedralBook::num_orders() const {
        return orders_.size();
    }

    std::vector<Fill> PolyhedralBook::add_order(Order o) {
        constexpr double kRemoveTol = 1e-9;
        assert(o.column.size() == static_cast<std::size_t>(N_));
        if(orders_.empty()) {
            rest_(std::move(o));
            return {};
        }
        //fast path predicate (no cross branch)
        if(y_cache_.has_value() && rests_under_prices(o.column, *y_cache_)){
            rest_(std::move(o));
            return {};
        }
        //loop coordinating phase 4 and 5 (check if cross may exist for a y')

        std::vector<Fill> fills;
        double remaining_M = 1.0;
        while (remaining_M > kRemoveTol){
            std::vector<std::vector<double>> cols = assemble_columns_();
            std::vector<double> z = o.column;
            for(double& v : z) v = v * remaining_M;
            //solving fair price dual
            FairPriceResult fp = solve_fair_price(cols, z, N_);
            assert(fp.status == HighsModelStatus::kOptimal && "dual infeasible; book already crossed");
            y_cache_ = fp.y; //y = y'
            //if z^t * y' < 0 then no more cross and re insert leftover
            if(fp.objective < 0.0){
                o.remaining = remaining_M;
                rest_(std::move(o));
                return fills;
            }
            // s^t * y' >= 0 so run the matching LP
            std::vector<std::vector<double>> active_cols;
            active_cols.reserve(fp.active.size());
            for(std::size_t i : fp.active){
                active_cols.push_back(cols[i]);
            }
            
            MatchingResult m = solve_matching(active_cols, z, N_);
            assert(m.status == HighsModelStatus::kOptimal);
            double factor = apply_matching_(m, fp.active, o.id, fills);
            assert(factor < 1.0 && "matching returned x_M = 0 despite cross; would infinite-loop");
            remaining_M *= factor;
        }
        return fills;
    }

    void PolyhedralBook::cancel(OrderId id) {
        if(orders_.erase(id)){
            std::erase(insertion_order_, id);
        }
    }

    void PolyhedralBook::seed_price_vector(std::vector<double> y) {
        assert(y.size() == static_cast<std::size_t>(N_));
        y_cache_ = std::move(y);
    }

    void PolyhedralBook::rest_(Order&& o){ //&& since using move command and gutting original
        insertion_order_.push_back(o.id);
        orders_.emplace(o.id, std::move(o)); //using move commands since we can discard o and we want speed (swapping pointers faster than copying)
    }

    std::vector<std::vector<double>> PolyhedralBook::assemble_columns_() const {
        std::vector<std::vector<double>> cols;
        cols.reserve(insertion_order_.size());
        for(OrderId id : insertion_order_){
            const Order& o = orders_.at(id);
            std::vector<double> scaled = o.column;
            for(double& s : scaled){
                s = s * o.remaining;
            }
            cols.push_back(std::move(scaled));
        }
        return cols;
    };

    double PolyhedralBook::price_of_(const std::vector<double>& column){
        assert(column.size() == 2 && "price_of_ assumes N >= 2");
        assert(column[0]!=0.0 && "asset-0 quantity is zero; not valid order column");
        return std::abs(column[1]/column[0]);
    }

    double PolyhedralBook::apply_matching_(const MatchingResult& m, const std::vector<std::size_t>& active, OrderId aggressor_id, std::vector<Fill>& fills_out){
        //Take in LP solution from solve_matching and turn it into Fill records while decrementing every resting order's remaining value
        constexpr double kRemovelTol = 1e-9;
        std::vector<OrderId> dead;

        for(std::size_t j = 0; j<active.size(); j++){
            std::size_t i = active[j];
            OrderId tightId = insertion_order_[i];
            Order& o = orders_.at(tightId);

            double scaled_quantity0 = std::abs(o.column[0]) * o.remaining; //needs to change when upgrade to N >= 3
            double fill_qty = m.x_active[j] * scaled_quantity0;
            fills_out.push_back(Fill {aggressor_id, tightId, price_of_(o.column), fill_qty});
            o.remaining *= (1.0 - m.x_active[j]);
            if(o.remaining < kRemovelTol) dead.push_back(tightId);
        }

        //erase dead orders (can be optimized later)
        for(OrderId id : dead) orders_.erase(id);
        if(!dead.empty()){
            std::vector<OrderId> still_alive;
            still_alive.reserve(insertion_order_.size());
            for(OrderId id : insertion_order_){
                if(orders_.contains(id)){
                    still_alive.push_back(id);
                }
            }
            insertion_order_ = std::move(still_alive);
        }
        return 1.0-m.x_M;
    }
    
    



}  // namespace pob::polyhedral
