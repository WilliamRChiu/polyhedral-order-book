#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <pob/polyhedral/pricing.hpp>
#include <Highs.h>

using pob::polyhedral::solve_fair_price;

TEST_CASE("fair price: empty book is optimal and produces a valid simplex point", "[pricing]"){
    // No resting orders. Only the ‖y‖₁ = 1 constraint applies.
    // For z = (-1, +50), the minimum of zᵀy on the simplex puts all weight
    // on the most-negative coordinate (asset 0), giving y = (1, 0), obj = -1.
    std::vector<std::vector<double>> columns;
    std::vector<double> z = {-1.0, 50.0};
    auto r = solve_fair_price(columns, z, 2);

    REQUIRE(r.status == HighsModelStatus::kOptimal);
    REQUIRE(r.active.empty());
    REQUIRE(r.objective == Catch::Approx(-1.0));
    REQUIRE(r.y.size() == 2);
    REQUIRE(r.y[0] + r.y[1] == Catch::Approx(1.0));
}

TEST_CASE("fair price: incoming buy at 51 crosses resting sell at 50", "[pricing]"){
    // Resting sell at 50 (counterparty view: +1 asset_0, -50 asset_1).
    // Incoming buy at 51 (counterparty view: -1 asset_0, +51 asset_1).
    // CLOB-crossed (bid 51 ≥ ask 50). Expect objective ≥ 0 (cross) and the
    // resting column is binding at the optimum (active = {0}).
    std::vector<std::vector<double>> columns = {{1.0, -50.0}};
    std::vector<double> z = {-1.0, 51.0};
    auto r = solve_fair_price(columns, z, 2);

    REQUIRE(r.status == HighsModelStatus::kOptimal);
    REQUIRE(r.objective >= 0.0);
    // Objective for this LP works out to 1/51 ≈ 0.0196.
    REQUIRE(r.objective == Catch::Approx(1.0 / 51.0));
    REQUIRE(r.active.size() == 1);
    REQUIRE(r.active[0] == 0);
}

TEST_CASE("fair price: incoming buy at 49 does not cross resting sell at 50", "[pricing]"){
    // Same resting sell at 50, but incoming buy at 49 (bid 49 < ask 50).
    // Expect objective < 0 (rest). The constraint y_0 ≤ 50 y_1 is still
    // tight at the optimum, so active = {0} — but the cross test is the
    // objective sign, not the active set.
    std::vector<std::vector<double>> columns = {{1.0, -50.0}};
    std::vector<double> z = {-1.0, 49.0};
    auto r = solve_fair_price(columns, z, 2);

    REQUIRE(r.status == HighsModelStatus::kOptimal);
    REQUIRE(r.objective < 0.0);
    REQUIRE(r.objective == Catch::Approx(-1.0 / 51.0));
}

TEST_CASE("fair price: two non-crossing sells, incoming buy below both rests", "[pricing]"){
    // Resting: sell at 50 and sell at 60 (both same side, never cross each other).
    // Incoming: buy at 40 (well below both asks). Should rest.
    std::vector<std::vector<double>> columns = {
        {1.0, -50.0},
        {1.0, -60.0},
    };
    std::vector<double> z = {-1.0, 40.0};
    auto r = solve_fair_price(columns, z, 2);

    REQUIRE(r.status == HighsModelStatus::kOptimal);
    REQUIRE(r.objective < 0.0);
    // Tighter sell at 50 sets the binding price; objective works out to -10/51.
    REQUIRE(r.objective == Catch::Approx(-10.0 / 51.0));
}
