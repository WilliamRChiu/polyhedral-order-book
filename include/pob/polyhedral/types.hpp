#pragma once
#include <cstdint>
#include <vector>

/*
Note, unlike CLOP, no side variable; the columns sign is the side
Also no price/quantity enums since polyhedral works in doubles natively since HiGHS outputs in doubles and columsn are vector<double>
*/
namespace pob::polyhedral{
    enum class OrderId : std::uint64_t{};
    enum class TimeStamp : std::int64_t{};

    struct Order {
        OrderId id;
        TimeStamp timeStamp;
        std::vector<double> column; //one order is one column in matrix A
        double remaining = 1.0; //minimize number of writes in one cycle of algorithm while also ensuring we can keep original data intact
    };

    struct Fill {
        OrderId aggressor_id;
        OrderId resting_id;
        double price; 
        double quantity;
    };
}