#include <pob/clob/order_book.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include "pob/clob/types.hpp"

namespace{
    using pob::clob::OrderBook;
    using pob::clob::Order;
    using pob::clob::OrderId;
    using pob::clob::Price;
    using pob::clob::Quantity;
    using pob::clob::Side;
    using pob::clob::TimeStamp;
    Order make(
        std::uint64_t id,
        Side side,
        std::int64_t price,
        std::int64_t quantity,
        std::int64_t timeStamp = 0
    ){
        return Order{OrderId{id}, side, Price{price}, Quantity{quantity}, TimeStamp{timeStamp}};
    }

}

TEST_CASE("empty book has no best bid or ask", "[orderbook]"){
    pob::clob::OrderBook book;
    REQUIRE_FALSE(book.best_bid().has_value());
    REQUIRE_FALSE(book.best_ask().has_value());
}


TEST_CASE("Test a order with more bids than available asks", "[orderbook]"){
    //In this test case, order will not be completely filled, bids_ should now have leftover appended, asks_ should have no orders less than or equal to order price level
    OrderBook book;
    book.add_limit(make(1, Side::Ask, 100, 2));
    book.add_limit(make(2, Side::Bid, 100, 5));
    REQUIRE_FALSE(book.best_ask().has_value());
    REQUIRE(book.best_bid() == Price{100});
}

TEST_CASE("Test a order with less bids than available asks", "[orderbook]"){
    //In this test case, entire order should be filled, bids_ size should stay the same
    OrderBook book;
    book.add_limit(make(1, Side::Ask, 100, 5));
    book.add_limit(make(2, Side::Bid, 100, 3));
    REQUIRE_FALSE(book.best_bid().has_value());
    REQUIRE(book.best_ask() == Price{100});
}

TEST_CASE("Test a order with more asks than available bids", "[orderbook]"){
    //In this test case, order will not be completely filled, asks_ should now have leftover order appended, bids_ should have no orders at or greater than the order price level
    OrderBook book;
    book.add_limit(make(1, Side::Bid, 110, 2));
    book.add_limit(make(2, Side::Ask, 100, 5));
    REQUIRE_FALSE(book.best_bid().has_value());
    REQUIRE(book.best_ask() == Price{100});
}

TEST_CASE("Test a order with less asks than available bids", "[orderbook]"){
    //In this test case, entire order should be filled, the asks_ size should stay the same
    OrderBook book;
    book.add_limit(make(1, Side::Bid, 110, 5));
    book.add_limit(make(2, Side::Ask, 100, 3));
    REQUIRE_FALSE(book.best_ask().has_value());
    REQUIRE(book.best_bid() == Price{110});
}