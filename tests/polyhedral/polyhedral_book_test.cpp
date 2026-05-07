#include "pob/polyhedral/types.hpp"
#include <pob/polyhedral/polyhedral_book.hpp>
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

TEST_CASE("empty polyhedral book with no orders", "[polyhedral][smoke]"){
    pob::polyhedral::PolyhedralBook book(2);
    REQUIRE(book.num_orders() == 0);
}

TEST_CASE("add_order increments num_orders", "[polyhedral]"){
    using namespace pob::polyhedral;
    PolyhedralBook book(2);

    book.add_order(Order{OrderId{1}, TimeStamp{0}, {1.0, -50.0}});
    REQUIRE(book.num_orders() == 1);

    book.add_order(Order{OrderId{2}, TimeStamp{1}, {-1.0, 51.0}});
    REQUIRE(book.num_orders() == 2);
}

TEST_CASE("cancel removes the order from the book", "[polyhedral]"){
    using namespace pob::polyhedral;
    PolyhedralBook book(2);

    book.add_order(Order{OrderId{1}, TimeStamp{0}, {1.0, -50.0}});
    book.cancel(OrderId{1});

    REQUIRE(book.num_orders() == 0);
}

TEST_CASE("seeded y: predicate rests order with negative dot product", "[polyhedral][bootstrap]"){
    // y = (1, 1), z = (1, -2)  →  yᵀz = -1 < 0  
    using namespace pob::polyhedral;
    PolyhedralBook book(2);
    book.seed_price_vector({1.0, 1.0});
    auto fills = book.add_order(Order{OrderId{1}, TimeStamp{0}, {1.0, -2.0}});
    REQUIRE(fills.empty());
    REQUIRE(book.num_orders() == 1);
}

TEST_CASE("seeded y: predicate falls through and LP stub throws", "[polyhedral][bootstrap]"){
    // y = (1, 1), z = (2, 1)  →  yᵀz = 3 ≥ 0  
    using namespace pob::polyhedral;
    PolyhedralBook book(2);
    book.seed_price_vector({1.0, 1.0});
    REQUIRE_THROWS_AS(
        book.add_order(Order{OrderId{1}, TimeStamp{0}, {2.0, 1.0}}),
        std::logic_error
    );
}