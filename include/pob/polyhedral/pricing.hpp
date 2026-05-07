#pragma once

#include "lp_data/HConst.h"
#include <vector>

//If fair pricing problem finishes with z^Ty < 0, then book is settled as by the paper, it means there is a price where no one wants to trade
//fair pricing problem is a modification of the dual, so solving it is a constructive way of showing dual feasibility
//We only will be able to trade orders in which when dot producted with our y vector, have a value of 0 as those are the only ones where the resting order does not lose money if used to execute trade
//we cannot trade bundle a order into trade if y^tA<0 as then the order is not being properly fulfilled (we make them sell or buy for a price out of their limit)
//By the no trade through clause we also can't trade existing orders on the book for more than what they asked for since in our model, all profits go to the aggressor



namespace pob::polyhedral {

struct FairPriceResult {
    HighsModelStatus status;
    std::vector<double> y; //price vector per good as do y^tA so map each order onto
    std::vector<std::size_t> active; //indices for columns (orders) where y^tA_i = 0  (dual is tight)
    double objective;
};



FairPriceResult solve_fair_price(
    const std::vector<std::vector<double>>& columns, //scaled book columns
    const std::vector<double>& z, //incoming order column
    int N
);

} // namespace pob::polyhedral