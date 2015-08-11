#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <utility>
#include <vector>

#include "LogicalProcess.hpp"
#include "mocks.hpp"

TEST_CASE("LogicalProcess support getState method", "[LogicalProcess]") {
    test_LogicalProcess lp {"lp", 1};
    test_LPState s {2};

    SECTION("lp state can be swapped through getState reference") {
        REQUIRE(static_cast<test_LPState&>(lp.getState()).x == 1);
        std::swap(static_cast<test_LPState&>(lp.getState()), s);
        REQUIRE(static_cast<test_LPState&>(lp.getState()).x == 2);
    }

    SECTION("lp state can be moved into getState reference") {
        REQUIRE(static_cast<test_LPState&>(lp.getState()).x == 1);
        static_cast<test_LPState&>(lp.getState()) = std::move(s);
        REQUIRE(static_cast<test_LPState&>(lp.getState()).x == 2);
    }
}

TEST_CASE("LogicalProcess support event methods", "[LogicalProcess]") {
    test_LogicalProcess lp {"lp", 1};
    test_Event e {"e", 1};

    SECTION("LogicalProcess's support receiveEvent") {
        auto out = lp.receiveEvent(e);
        REQUIRE(out.size() == 1);
        CHECK(out[0]->receiverName() == "e");
        REQUIRE(out[0]->timestamp() == 1);
    }

    SECTION("LogicalProcess;s support default createInitialEvents") {
        REQUIRE(lp.initializeLP().size() == 0);
    }
}
