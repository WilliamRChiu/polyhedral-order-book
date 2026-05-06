#include <pob/polyhedral/polyhedral_book.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("empty polyhedral book with no orders", "[polyhedral][smoke]"){
    pob::polyhedral::PolyhedralBook book(2);
    REQUIRE(book.num_orders() == 0);
}