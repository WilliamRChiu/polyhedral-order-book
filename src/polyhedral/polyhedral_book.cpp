#include <pob/polyhedral/polyhedral_book.hpp>
#include <cassert>
#include <pob/polyhedral/predicates.hpp>
#include <stdexcept>

namespace pob::polyhedral {

    std::size_t PolyhedralBook::num_orders() const {
        return orders_.size();
    }

    std::vector<Fill> PolyhedralBook::add_order(Order o) {
        assert(o.column.size() == static_cast<std::size_t>(N_));
        if(!y_cache_.has_value()) {
            rest_(std::move(o));
            return {};
        }
        if(rests_under_prices(o.column, *y_cache_)){
            rest_(std::move(o));
            return {};
        }
        //loop coordinating phase 4 and 5
        throw std::logic_error("LP path not implemented");
    }

    void PolyhedralBook::cancel(OrderId id) {
        orders_.erase(id);
    }

    void PolyhedralBook::seed_price_vector(std::vector<double> y) {
        assert(y.size() == static_cast<std::size_t>(N_));
        y_cache_ = std::move(y);
    }

    void PolyhedralBook::rest_(Order&& o){ //&& since using move command and gutting original
        insertion_order_.push_back(o.id);
        orders_.emplace(o.id, std::move(o)); //using move commands since we can discard o and we want speed (swapping pointers faster than copying)
    }

}  // namespace pob::polyhedral
