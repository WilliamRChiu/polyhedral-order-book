#include <catch2/catch_test_macros.hpp>
#include <Highs.h>
#include <catch2/catch_approx.hpp>


TEST_CASE("Smoke test", "[highs][smoke]"){
    //buy is [1, -50]^t and sell is [-1, +51]^t where first element is shares of abc and second element is shares USD (i.e. dollars)
    HighsLp lp;
    lp.num_col_ = 2;
    lp.num_row_ = 2;

    // Maximize x_buy + x_sell.
    lp.sense_ = ObjSense::kMaximize;
    lp.col_cost_ = {1.0, 1.0};

    // 0 <= x_i <= 1.
    lp.col_lower_ = {0.0, 0.0};
    lp.col_upper_ = {1.0, 1.0};

    // Ax >= 0 (no upper bound).
    lp.row_lower_ = {0.0, 0.0};
    lp.row_upper_ = {kHighsInf, kHighsInf};

    // A in CSC (Compressed Sparse Column -> Sotre only non zero numbers):
    //         buy   sell
    //   a0 [  +1    -1  ]
    //   a1 [ -50   +51  ]
    lp.a_matrix_.format_ = MatrixFormat::kColwise;
    lp.a_matrix_.start_  = {0, 2, 4};
    lp.a_matrix_.index_  = {0, 1, 0, 1};
    lp.a_matrix_.value_  = {1.0, -50.0, -1.0, 51.0};

    Highs h;
    h.setOptionValue("output_flag", false);  // silence solver log during tests
    REQUIRE(h.passModel(lp) == HighsStatus::kOk);
    REQUIRE(h.run() == HighsStatus::kOk);
    REQUIRE(h.getModelStatus() == HighsModelStatus::kOptimal);

    const HighsSolution& sol = h.getSolution();

    // Both orders fully execute.
    REQUIRE(sol.col_value[0] == Catch::Approx(1.0)); //x0 = 1
    REQUIRE(sol.col_value[1] == Catch::Approx(1.0)); //x1 = 1

    // Asset 0 nets to zero; asset 1 has +$1 surplus (the price improvement).
    REQUIRE(sol.row_value[0] == Catch::Approx(0.0)); //b0 = 0
    REQUIRE(sol.row_value[1] == Catch::Approx(1.0)); //b1 = 1

    // Objective = total executed volume = 2.
    REQUIRE(h.getObjectiveValue() == Catch::Approx(2.0));
}