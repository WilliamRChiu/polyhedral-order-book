#include "Highs.h"
#include "lp_data/HConst.h"
#include "lp_data/HighsLp.h"
#include "lp_data/HighsStatus.h"
#include <cassert>
#include <cmath>
#include <numeric>
#include <pob/polyhedral/pricing.hpp>

namespace pob::polyhedral{
    FairPriceResult solve_fair_price(const std::vector<std::vector<double>> &columns, const std::vector<double> &z, int N){
        assert(z.size() == static_cast<std::size_t>(N));
        for(std::size_t i = 0; i<columns.size(); i++){
            assert(columns[i].size() == static_cast<std::size_t>(N) && "column dimension mismatch");
        };
        HighsLp lp;
        lp.num_col_ = N;
        lp.num_row_ = columns.size() + 1; //+1 to encode ||y|| = 1
        lp.sense_ = ObjSense::kMinimize;
        lp.col_cost_ = z; //obj is min z^ty
        lp.col_lower_ = std::vector<double>(N,0.0); //y>=0
        lp.col_upper_ = std::vector<double>(N,kHighsInf); // y upper unbounded
        lp.row_lower_ = std::vector<double>(columns.size(), -kHighsInf);
        lp.row_lower_.push_back(1.0);
        lp.row_upper_ = std::vector<double>(columns.size(), 0.0);
        lp.row_upper_.push_back(1.0);


        //Building Compressed Sparse Matrix
        const std::size_t M = columns.size();
        lp.a_matrix_.format_ = MatrixFormat::kColwise;
        lp.a_matrix_.index_.reserve(static_cast<std::size_t>(N) * (M + 1));
        lp.a_matrix_.start_.reserve(N + 1);
        lp.a_matrix_.value_.reserve(static_cast<std::size_t>(N) * (M + 1));

        lp.a_matrix_.start_ = {0};
        for(int n = 0; n < N; n++){
            for(std::size_t m = 0; m < M; m++){
                lp.a_matrix_.index_.push_back(static_cast<int>(m));   //row = book column m
                lp.a_matrix_.value_.push_back(columns[m][n]);          //asset n's entry for that column
            }
            lp.a_matrix_.index_.push_back(static_cast<int>(M));        //normalization row (||y|| = 1)
            lp.a_matrix_.value_.push_back(1.0);                        
            lp.a_matrix_.start_.push_back(lp.a_matrix_.start_.back() + static_cast<int>(M) + 1);
        }

        assert(lp.a_matrix_.start_.size() == static_cast<std::size_t>(N) + 1);
        assert(lp.a_matrix_.index_.size() == lp.a_matrix_.value_.size());
        assert(lp.a_matrix_.value_.size() == static_cast<std::size_t>(N) * (M + 1));

        Highs h;
        h.setOptionValue("output_flag", false);
        HighsStatus pass_status = h.passModel(lp);
        HighsStatus run_status = h.run();
        HighsModelStatus model_status = h.getModelStatus();


        FairPriceResult result;
        result.status = model_status;
        if(model_status == HighsModelStatus::kOptimal){
            const HighsSolution& sol = h.getSolution();
            result.y = sol.col_value;
            result.objective = h.getObjectiveValue(); //z^ty value

            //Filling out Active Set for book columns with y^tA_i = 0 (estimation due to doubles and HiGHS)
            constexpr double kActiveTol = 1e-9;
            for (std::size_t i = 0; i < M; i++) {
                const double slack_i = std::inner_product(
                    result.y.begin(), result.y.end(), columns[i].begin(), 0.0);
                if (std::abs(slack_i) < kActiveTol) {
                    result.active.push_back(i);
                }
            }
        }
        return result;
    };
}