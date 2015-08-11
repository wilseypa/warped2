#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>

#include "LPState.hpp"
#include "mocks.hpp"

TEST_CASE("The State can be derived from", "[LPState]") {
    test_LPState s1 {1};

    SECTION("subclasses can be cloned") {
        auto ps2 = s1.clone();
        auto s2 = static_cast<test_LPState&>(*ps2);
        REQUIRE(s2.x == 1);
        s2.x = 2;
        CHECK(s1.x == 1);
        REQUIRE(s2.x == 2);
    }

    SECTION("subclasses can be cloned from base pointer") {
        warped::LPState* ps1 = &s1;
        auto ps2 = ps1->clone();
        auto s2 = static_cast<test_LPState&>(*ps2);
        REQUIRE(s2.x == 1);
        s2.x = 2;
        CHECK(s1.x == 1);
        REQUIRE(s2.x == 2);
    }
}
