#include "lp_data/HConst.h"
#include <vector>
#pragma once

namespace pob::polyhedral{
    struct MatchingResult {
        HighsModelStatus status;
        std::vector<double> x_active; //should have k entries with same order as the input active columns
        double x_M; //fraction of the incoming order that is executed on
    };

    MatchingResult solve_matching(
        const std::vector<std::vector<double>>& active_columns,
        const std::vector<double>& z,
        int N
    );
}
