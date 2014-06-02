#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <utility>
#include <vector>

#include "SimulationObject.hpp"
#include "mocks.hpp"

TEST_CASE("SimualtionObjects support getState method", "[SimulationObject]") {
    test_SimulationObject ob {"ob", 1};
    test_ObjectState s {2};

    SECTION("object state can be swapped through getState reference") {
        REQUIRE(static_cast<test_ObjectState&>(ob.getState()).x == 1);
        std::swap(static_cast<test_ObjectState&>(ob.getState()), s);
        REQUIRE(static_cast<test_ObjectState&>(ob.getState()).x == 2);
    }

    SECTION("object state can be moved into getState reference") {
        REQUIRE(static_cast<test_ObjectState&>(ob.getState()).x == 1);
        static_cast<test_ObjectState&>(ob.getState()) = std::move(s);
        REQUIRE(static_cast<test_ObjectState&>(ob.getState()).x == 2);
    }
}

TEST_CASE("SimualtionObjects support event methods", "[SimulationObject]") {
    test_SimulationObject ob {"ob", 1};
    test_Event e {"e", 1};

    SECTION("SimualtionObjects support receiveEvent") {
        auto out = ob.receiveEvent(e);
        REQUIRE(out.size() == 1);
        CHECK(out[0]->receiverName() == "e");
        REQUIRE(out[0]->timestamp() == 1);
    }

    SECTION("SimualtionObjects support default createInitialEvents") {
        REQUIRE(ob.createInitialEvents().size() == 0);
    }
}