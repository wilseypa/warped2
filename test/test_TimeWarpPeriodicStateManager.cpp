#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>

#include "TimeWarpPeriodicStateManager.hpp"
#include "mocks.hpp"

TEST_CASE("States saved and restored correctly", "[state][queue]") {
    unsigned int num_objects = 4;
    unsigned int period = 2;
    warped::TimeWarpPeriodicStateManager sm(period);
    sm.initialize(num_objects);
    warped::SimulationObject *object;
    std::shared_ptr<warped::Event> e = std::make_shared<test_Event>();

    SECTION("Periodic state manager constructed correctly", "[]") {
        for (unsigned int i = 0; i < num_objects; i++) {
            REQUIRE(sm.size(i) == 0);
        }
    }

    object = new test_SimulationObject("test_object", 50);

    // All of the following tests are for object_id = 3
    SECTION("States are saved for first call to saveState", "[state][save]") {
        static_cast<test_ObjectState&>(object->getState()).x++; // 51
        dynamic_cast<test_Event*>(e.get())->receive_time_ = 15;
        sm.saveState(e, 3, object); // timestamp 15
        REQUIRE(sm.size(3) == 1);   // Saved

        SECTION("States are saved in correct periods", "[state][save]") {
            static_cast<test_ObjectState&>(object->getState()).x++; // 52
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 20;
            sm.saveState(e, 3, object); // timestamp 20
            REQUIRE(sm.size(3) == 1); // Not saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 53
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 25;
            sm.saveState(e, 3, object); // timestamp 25
            REQUIRE(sm.size(3) == 2); // Saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 54
            sm.saveState(e, 3, object); // timestamp 25
            REQUIRE(sm.size(3) == 2); // Not saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 55
            sm.saveState(e, 3, object); // timestamp 25
            REQUIRE(sm.size(3) == 3); // Saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 56
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 30;
            sm.saveState(e, 3, object); // timestamp 30
            REQUIRE(sm.size(3) == 3); // Not saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 57
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 33;
            sm.saveState(e, 3, object); // timestamp 33
            REQUIRE(sm.size(3) == 4); // Saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 58
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 35;
            sm.saveState(e, 3, object); // timestamp 35
            REQUIRE(sm.size(3) == 4); // Not saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 59
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 37;
            sm.saveState(e, 3, object); // timestamp 37
            REQUIRE(sm.size(3) == 5); // Saved

            SECTION("Fossil collection removes correct states", "[state][fossilCollection]") {
                sm.fossilCollect(28, 3);
                REQUIRE(sm.size(3) == 2);
            }

            SECTION("State can be restored on rollback", "[state][rollback][restore]") {
                dynamic_cast<test_Event*>(e.get())->receive_time_ = 26;
                std::shared_ptr<warped::Event> restored_state_event = sm.restoreState(e, 3, object);
                REQUIRE(sm.size(3) == 3);
                REQUIRE(restored_state_event->timestamp() == 25);
                REQUIRE(static_cast<test_ObjectState&>(object->getState()).x == 55);
            }
        }
    }
}



