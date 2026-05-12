#include "pob/polyhedral/types.hpp"
#include <pob/polyhedral/polyhedral_book.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("empty polyhedral book with no orders", "[polyhedral][smoke]"){
    pob::polyhedral::PolyhedralBook book(2);
    REQUIRE(book.num_orders() == 0);
}

TEST_CASE("add_order increments num_orders", "[polyhedral]"){
    // two same-side sells never cross, so both rest and num_orders climbs
    using namespace pob::polyhedral;
    PolyhedralBook book(2);

    book.add_order(Order{OrderId{1}, TimeStamp{0}, {1.0, -50.0}});
    REQUIRE(book.num_orders() == 1);

    book.add_order(Order{OrderId{2}, TimeStamp{1}, {1.0, -55.0}});
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

TEST_CASE("add_order: full cross between resting sell at 50 and buy at 51", "[polyhedral][phase6]"){
    // sell at 50 rests, then buy at 51 crosses and fully fills
    using namespace pob::polyhedral;
    PolyhedralBook book(2);

    auto fills_a = book.add_order(Order{OrderId{1}, TimeStamp{0}, {1.0, -50.0}});
    REQUIRE(fills_a.empty());

    auto fills_b = book.add_order(Order{OrderId{2}, TimeStamp{1}, {-1.0, 51.0}});
    REQUIRE(fills_b.size() == 1);
    REQUIRE(fills_b[0].aggressor_id == OrderId{2});
    REQUIRE(fills_b[0].resting_id == OrderId{1});
    REQUIRE(fills_b[0].price == Catch::Approx(50.0));
    REQUIRE(fills_b[0].quantity == Catch::Approx(1.0));
    REQUIRE(book.num_orders() == 0);
}

TEST_CASE("add_order: non-crossing buy through LP path rests", "[polyhedral][phase6]"){
    // sell at 50 rests, buy at 49 does not cross, LP routes to rest
    using namespace pob::polyhedral;
    PolyhedralBook book(2);

    book.add_order(Order{OrderId{1}, TimeStamp{0}, {1.0, -50.0}});
    auto fills = book.add_order(Order{OrderId{2}, TimeStamp{1}, {-1.0, 49.0}});

    REQUIRE(fills.empty());
    REQUIRE(book.num_orders() == 2);
}

TEST_CASE("add_order: aggressor consumes resting and leftover rests", "[polyhedral][phase6]"){
    // sell qty 1 at 50, buy qty 2 at 51, 1 unit fills, leftover buy stays
    using namespace pob::polyhedral;
    PolyhedralBook book(2);

    book.add_order(Order{OrderId{1}, TimeStamp{0}, {1.0, -50.0}});
    auto fills = book.add_order(Order{OrderId{2}, TimeStamp{1}, {-2.0, 102.0}});

    REQUIRE(fills.size() == 1);
    REQUIRE(fills[0].aggressor_id == OrderId{2});
    REQUIRE(fills[0].resting_id == OrderId{1});
    REQUIRE(fills[0].price == Catch::Approx(50.0));
    REQUIRE(fills[0].quantity == Catch::Approx(1.0));
    REQUIRE(book.num_orders() == 1);
}

TEST_CASE("add_order: aggressor walks two resting levels", "[polyhedral][phase6]"){
    // sells at 50 and 51 both qty 1, buy qty 2 at 52 takes both
    using namespace pob::polyhedral;
    PolyhedralBook book(2);

    book.add_order(Order{OrderId{1}, TimeStamp{0}, {1.0, -50.0}});
    book.add_order(Order{OrderId{2}, TimeStamp{1}, {1.0, -51.0}});
    auto fills = book.add_order(Order{OrderId{3}, TimeStamp{2}, {-2.0, 104.0}});

    REQUIRE(fills.size() == 2);
    REQUIRE(fills[0].price == Catch::Approx(50.0));
    REQUIRE(fills[0].quantity == Catch::Approx(1.0));
    REQUIRE(fills[1].price == Catch::Approx(51.0));
    REQUIRE(fills[1].quantity == Catch::Approx(1.0));
    REQUIRE(book.num_orders() == 0);
}

TEST_CASE("add_order: non-crossing later order takes predicate fast-path", "[polyhedral][phase6]"){
    // partial cross leaves the book non-empty and y_cache primed,
    // so the next clearly non-crossing order routes via rests_under_prices
    // instead of falling back into the LP path
    using namespace pob::polyhedral;
    PolyhedralBook book(2);

    book.add_order(Order{OrderId{1}, TimeStamp{0}, {2.0, -100.0}});  // sell qty 2 at 50
    book.add_order(Order{OrderId{2}, TimeStamp{1}, {-1.0, 51.0}});   // buy qty 1 at 51, partial cross
    // resting sell now has remaining = 0.5, y_cache primed, book non-empty

    auto fills = book.add_order(Order{OrderId{3}, TimeStamp{2}, {1.0, -60.0}});  // sell at 60, far above cross
    REQUIRE(fills.empty());
    REQUIRE(book.num_orders() == 2);
}