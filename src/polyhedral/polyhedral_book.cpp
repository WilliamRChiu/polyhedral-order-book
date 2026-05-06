#include <pob/polyhedral/polyhedral_book.hpp>

namespace pob::polyhedral {

std::size_t PolyhedralBook::num_orders() const {
    return orders_.size();
}

std::vector<Fill> PolyhedralBook::add_order(Order o) {
    orders_.emplace(o.id, std::move(o));
    return {};
}

void PolyhedralBook::cancel(OrderId id) {
    orders_.erase(id);
}

}  // namespace pob::polyhedral
