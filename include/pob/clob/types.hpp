#pragma once
#include <cstdint>

namespace pob::clob {
    enum class Price : std::int64_t {};
    enum class Side { Bid, Ask };
    enum class Quantity : std::int64_t {};
    enum class OrderId : std::uint64_t {}; //unsigned to give double range and wraparounds become predictable in behaviour; no math run on this
    enum class TimeStamp : std::int64_t {};
    struct Order {
        OrderId id;
        Side side;
        Price price;
        Quantity quantity;
        TimeStamp timestamp;
    };

    struct Fill {
        OrderId agressor_id; //order triggering the match
        OrderId resting_id; //order that was sitting in book and got matched
        Price price; //price at which the match occurred (resting order price)
        Quantity quantity; //amount traded in this match
    };
}  