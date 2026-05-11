#include "Highs.h"
#include "lp_data/HConst.h"
#include "lp_data/HStruct.h"
#include "lp_data/HighsLp.h"
#include "lp_data/HighsStatus.h"
#include <cassert>
#include <pob/polyhedral/matching.hpp>
#include <vector>


namespace pob::polyhedral{
    MatchingResult solve_matching(const std::vector<std::vector<double>> &active_columns, const std::vector<double> &z, int N){
        int k = active_columns.size();
        for(std::size_t i = 0; i<static_cast<std::size_t>(k); i++){
            assert(active_columns[i].size() ==static_cast<std::size_t>(N) && "column dimension mismatch");
        };
        HighsLp lp;
        lp.num_col_ = k + 1;
        lp.num_row_ = N;
        lp.sense_ = ObjSense::kMaximize;
        lp.col_cost_.assign(k, 0.0);
        lp.col_cost_.push_back(1.0);
        lp.col_lower_ = std::vector<double>(k+1, 0.0);
        lp.col_upper_ = std::vector<double>(k+1, 1.0);
        lp.row_lower_ = std::vector<double>(N, 0.0);
        lp.row_upper_ = std::vector<double>(N, kHighsInf);


        //building matrix of tight constraints + new entering z order
        lp.a_matrix_.format_ = MatrixFormat::kColwise;
        lp.a_matrix_.index_.reserve((k+1) * static_cast<std::size_t>(N));
        lp.a_matrix_.value_.reserve((k+1) * static_cast<std::size_t>(N));
        lp.a_matrix_.start_.reserve(k+2);
        lp.a_matrix_.start_ = {0};
        for(std::size_t j = 0; j<static_cast<std::size_t>(k+1); j++){
            const std::vector<double>& source = (j < k) ? active_columns[j] : z;
            for(int n = 0; n < N; n++){
                lp.a_matrix_.index_.push_back(n);
                lp.a_matrix_.value_.push_back(source[n]);
            }
            lp.a_matrix_.start_.push_back(lp.a_matrix_.start_.back() + N);
        }

        assert(lp.a_matrix_.start_.size() == k+2);
        assert(lp.a_matrix_.index_.size() == lp.a_matrix_.value_.size());
        assert(lp.a_matrix_.value_.size() == (k+1) * static_cast<std::size_t>(N));

        Highs h;
        h.setOptionValue("output_flag", false);
        HighsStatus pass_status = h.passModel(lp);
        HighsStatus run_status = h.run();
        HighsModelStatus model_status = h.getModelStatus();

        MatchingResult result;
        result.status = model_status;
        if(result.status == HighsModelStatus::kOptimal){
            const HighsSolution& sol = h.getSolution();
            result.x_active.assign(sol.col_value.begin(), sol.col_value.begin() + active_columns.size());
            result.x_M = sol.col_value[k];
        }
        return result;
    };
}