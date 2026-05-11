#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <pob/polyhedral/matching.hpp>
#include <Highs.h>

using pob::polyhedral::solve_matching;

TEST_CASE("matching: basic full cross between resting sell at 50 and buy at 51", "[matching]"){
    // Resting sell at 50 (column [+1, -50]), incoming buy at 51 (z = [-1, 51]).
    std::vector<std::vector<double>> active_columns = {{1.0, -50.0}};
    std::vector<double> z = {-1.0, 51.0};
    auto r = solve_matching(active_columns, z, 2);

    REQUIRE(r.status == HighsModelStatus::kOptimal);
    REQUIRE(r.x_M == Catch::Approx(1.0));
    REQUIRE(r.x_active.size() == 1);
    REQUIRE(r.x_active[0] == Catch::Approx(1.0));
}

TEST_CASE("matching: aggressor partially fills against smaller resting remainder", "[matching]"){
    // Resting sell at 50 already half-consumed: column scaled to [+0.5, -25].
    // Incoming buy at 51 (z = [-1, 51]).
    std::vector<std::vector<double>> active_columns = {{0.5, -25.0}};
    std::vector<double> z = {-1.0, 51.0};
    auto r = solve_matching(active_columns, z, 2);

    REQUIRE(r.status == HighsModelStatus::kOptimal);
    REQUIRE(r.x_M == Catch::Approx(0.5));
    REQUIRE(r.x_active.size() == 1);
    REQUIRE(r.x_active[0] == Catch::Approx(1.0));
}

TEST_CASE("matching: large aggressor exhausts smaller resting and fills partially", "[matching]"){
    // Same half-remaining resting [+0.5, -25].
    // Larger incoming buy: Q=2 at 51 -> z = [-2, 102].
    std::vector<std::vector<double>> active_columns = {{0.5, -25.0}};
    std::vector<double> z = {-2.0, 102.0};
    auto r = solve_matching(active_columns, z, 2);

    REQUIRE(r.status == HighsModelStatus::kOptimal);
    REQUIRE(r.x_M == Catch::Approx(0.25));
    REQUIRE(r.x_active.size() == 1);
    REQUIRE(r.x_active[0] == Catch::Approx(1.0));
}

TEST_CASE("matching: empty active set returns zero fill without crashing", "[matching]"){
    // Degenerate case: no active resting columns. The only variable is x_M, and
    // z = [-1, 51] forces x_M <= 0 via asset 0's row. Should solve cleanly to 0.
    // Phase 6 normally won't invoke matching with empty active (z^Ty < 0 means
    // rest), but solve_matching should not crash if it happens.
    std::vector<std::vector<double>> active_columns;
    std::vector<double> z = {-1.0, 51.0};
    auto r = solve_matching(active_columns, z, 2);

    REQUIRE(r.status == HighsModelStatus::kOptimal);
    REQUIRE(r.x_M == Catch::Approx(0.0));
    REQUIRE(r.x_active.empty());
}

TEST_CASE("matching: aggressor walks two resting levels at 50 and 51", "[matching]"){
    // Two resting sells: at 50 ([+1, -50]) and at 51 ([+1, -51]).
    // Incoming buy Q=2 at 52: z = [-2, 104].
    std::vector<std::vector<double>> active_columns = {
        {1.0, -50.0},
        {1.0, -51.0},
    };
    std::vector<double> z = {-2.0, 104.0};
    auto r = solve_matching(active_columns, z, 2);

    REQUIRE(r.status == HighsModelStatus::kOptimal);
    REQUIRE(r.x_M == Catch::Approx(1.0));
    REQUIRE(r.x_active.size() == 2);
    REQUIRE(r.x_active[0] == Catch::Approx(1.0));
    REQUIRE(r.x_active[1] == Catch::Approx(1.0));
}
