#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <utility>
#include <vector>

#include "SimulationObject.hpp"

#include "Event.hpp"
#include "ObjectState.hpp"

Define_ObjectState_Subclass(test_ObjectState) {
public:
    test_ObjectState(int x): x(x) {}
    int x;
};

struct test_Event : warped::Event {
    test_Event() = default;
    test_Event(const std::string& receiver_name, unsigned int receive_time)
        : receiver_name_(receiver_name), receive_time_(receive_time) {}

    const std::string& get_receiver_name() {return receiver_name_;}
    unsigned int get_receive_time() {return receive_time_;}

    std::string receiver_name_;
    unsigned int receive_time_;
};

class test_SimulationObject : public warped::SimulationObject {
public:
    test_SimulationObject(const std::string& name, int x)
        : SimulationObject(name), state(x) {}

    warped::ObjectState& getState() { return state; }

    std::vector<std::unique_ptr<warped::Event>> receiveEvent(warped::Event* event) {
        std::vector<std::unique_ptr<warped::Event>> v;
        v.emplace_back(new test_Event(event->get_receiver_name(), event->get_receive_time()));
        return v;
    }

private:
    test_ObjectState state;
};


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
        auto out = ob.receiveEvent(&e);
        REQUIRE(out.size() == 1);
        CHECK(out[0]->get_receiver_name() == "e");
        REQUIRE(out[0]->get_receive_time() == 1);
    }

    SECTION("SimualtionObjects support default createInitialEvents") {
        REQUIRE(ob.createInitialEvents().size() == 0);
    }
}