#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>

#include "PeriodicStateManager.hpp"
#include "mocks.hpp"

TEST_CASE("States saved and restored correctly", "[state][queue]") {
    unsigned int num_objects = 4;
    unsigned int period = 2;
    warped::PeriodicStateManager sm(num_objects, period);
    warped::SimulationObject *object;

    SECTION("Periodic state manager constructed correctly", "[]") {
        for (unsigned int i = 0; i < num_objects; i++) {
            REQUIRE(sm.size(i) == 0);
        }
    }

    object = new test_SimulationObject("test_object", 50);

    // All of the following tests are for object_id = 3
    SECTION("States are saved for first call to saveState", "[state][save]") {
        static_cast<test_ObjectState&>(object->getState()).x++; // 51
        sm.saveState(15, 3, object); // timestamp 15
        REQUIRE(sm.size(3) == 1);   // Saved

        SECTION("States are saved in correct periods", "[state][save]") {
            static_cast<test_ObjectState&>(object->getState()).x++; // 52
            sm.saveState(20, 3, object); // timestamp 20
            REQUIRE(sm.size(3) == 1); // Not saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 53
            sm.saveState(25, 3, object); // timestamp 25
            REQUIRE(sm.size(3) == 2); // Saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 54
            sm.saveState(25, 3, object); // timestamp 25
            REQUIRE(sm.size(3) == 2); // Not saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 55
            sm.saveState(25, 3, object); // timestamp 25
            REQUIRE(sm.size(3) == 3); // Saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 56
            sm.saveState(30, 3, object); // timestamp 30
            REQUIRE(sm.size(3) == 3); // Not saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 57
            sm.saveState(33, 3, object); // timestamp 33
            REQUIRE(sm.size(3) == 4); // Saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 58
            sm.saveState(35, 3, object); // timestamp 35
            REQUIRE(sm.size(3) == 4); // Not saved

            static_cast<test_ObjectState&>(object->getState()).x++; // 59
            sm.saveState(37, 3, object); // timestamp 37
            REQUIRE(sm.size(3) == 5); // Saved

            SECTION("Fossil collection removes correct states", "[state][fossilCollection]") {
                unsigned int lts = sm.fossilCollect(28, 3);
                REQUIRE(sm.size(3) == 2);
                REQUIRE(lts == 33);
            }

            SECTION("State can be restored on rollback", "[state][rollback][restore]") {
                unsigned int saved_timestamp = sm.restoreState(26, 3, object);
                REQUIRE(sm.size(3) == 3);
                REQUIRE(saved_timestamp == 25);
                REQUIRE(static_cast<test_ObjectState&>(object->getState()).x == 55);
            }
        }
    }
}


