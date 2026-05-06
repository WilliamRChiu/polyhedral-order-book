#pragma once
#include <vector>
#include <cassert>
#include <numeric>


namespace pob::polyhedral{
    inline bool rests_under_prices(const std::vector<double>& z, const std::vector<double>& y){
        //calculate y^tz
        assert(z.size()==y.size());
        double sum = std::inner_product(z.begin(), z.end(), y.begin(), 0.0);
        return sum < 0.0; //fall through to LP as true if sum >=0 (aka the dual shows a upper bound that may be feasible)
    };
}