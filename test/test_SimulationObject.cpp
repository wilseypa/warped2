#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <SimulationObject.hpp>
#include <ObjectState.hpp>
#include <utility>

Define_ObjectState_Subclass(test_ObjectState) {
public:
    test_ObjectState(int x): x(x) {}
    int x;
};

class test_SimulationObject : public warped::SimulationObject {
public:
    test_SimulationObject(const std::string& name, int x)
        : SimulationObject(name), state(x) {}
    warped::ObjectState& getState() { return state; }

private:
    test_ObjectState state;
};


TEST_CASE("Basic SimualtionObject fucntionality", "[SimulationObject]") {
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