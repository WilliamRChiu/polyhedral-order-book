#include <catch2/catch_test_macros.hpp>
#include<vector>
#include <pob/polyhedral/predicates.hpp>

TEST_CASE("Check if negative dot product returns true", "[predicates][smoke]"){
    std::vector<double> z = {1, -2};
    std::vector<double> y = {1,1};
    REQUIRE(pob::polyhedral::rests_under_prices(z, y));

}

TEST_CASE("Check if postive dot product returns false", "[predicates][smoke]"){
    std::vector<double> z = {2,1};
    std::vector<double> y = {1,1};
    REQUIRE_FALSE(pob::polyhedral::rests_under_prices(z,y));
}

TEST_CASE("Check if zero dot product return false", "[predicates][smoke]"){
    std::vector<double> z = {1, -1};
    std::vector<double> y = {1,1};
    REQUIRE_FALSE(pob::polyhedral::rests_under_prices(z,y));
}