#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>

#include "TimeWarpPeriodicStateManager.hpp"
#include "mocks.hpp"

TEST_CASE("States saved and restored correctly", "[state][queue]") {
    unsigned int num_lps = 4;
    unsigned int period = 2;
    warped::TimeWarpPeriodicStateManager sm(period);
    sm.initialize(num_lps);
    warped::LogicalProcess *lp;
    std::shared_ptr<warped::Event> e;

    SECTION("Periodic state manager constructed correctly", "[]") {
        for (unsigned int i = 0; i < num_lps; i++) {
            REQUIRE(sm.size(i) == 0);
        }
    }

    lp = new test_LogicalProcess("test_lp", 50);

    // All of the following tests are for lp_id = 3
    SECTION("States are saved for first call to saveState", "[state][save]") {
        static_cast<test_LPState&>(lp->getState()).x++; // 51
        e = std::make_shared<test_Event>();
        dynamic_cast<test_Event*>(e.get())->receive_time_ = 15;
        sm.saveState(e, 3, lp); // timestamp 15
        REQUIRE(sm.size(3) == 1);   // Saved

        SECTION("States are saved in correct periods", "[state][save]") {
            static_cast<test_LPState&>(lp->getState()).x++; // 52
            e = std::make_shared<test_Event>();
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 20;
            sm.saveState(e, 3, lp); // timestamp 20
            REQUIRE(sm.size(3) == 1); // Not saved

            static_cast<test_LPState&>(lp->getState()).x++; // 53
            e = std::make_shared<test_Event>();
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 25;
            sm.saveState(e, 3, lp); // timestamp 25
            REQUIRE(sm.size(3) == 2); // Saved

            static_cast<test_LPState&>(lp->getState()).x++; // 54
            e = std::make_shared<test_Event>();
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 25;
            sm.saveState(e, 3, lp); // timestamp 25
            REQUIRE(sm.size(3) == 2); // Not saved

            static_cast<test_LPState&>(lp->getState()).x++; // 55
            e = std::make_shared<test_Event>();
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 25;
            sm.saveState(e, 3, lp); // timestamp 25
            REQUIRE(sm.size(3) == 3); // Saved

            static_cast<test_LPState&>(lp->getState()).x++; // 56
            e = std::make_shared<test_Event>();
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 30;
            sm.saveState(e, 3, lp); // timestamp 30
            REQUIRE(sm.size(3) == 3); // Not saved

            static_cast<test_LPState&>(lp->getState()).x++; // 57
            e = std::make_shared<test_Event>();
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 33;
            sm.saveState(e, 3, lp); // timestamp 33
            REQUIRE(sm.size(3) == 4); // Saved

            static_cast<test_LPState&>(lp->getState()).x++; // 58
            e = std::make_shared<test_Event>();
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 35;
            sm.saveState(e, 3, lp); // timestamp 35
            REQUIRE(sm.size(3) == 4); // Not saved

            static_cast<test_LPState&>(lp->getState()).x++; // 59
            e = std::make_shared<test_Event>();
            dynamic_cast<test_Event*>(e.get())->receive_time_ = 37;
            sm.saveState(e, 3, lp); // timestamp 37
            REQUIRE(sm.size(3) == 5); // Saved

            SECTION("Fossil collection removes correct states", "[state][fossilCollection]") {
                sm.fossilCollect(28, 3);
                REQUIRE(sm.size(3) == 3);
            }

            SECTION("State can be restored on rollback", "[state][rollback][restore]") {
                REQUIRE(sm.size(3) == 5);
                e = std::make_shared<test_Event>();
                dynamic_cast<test_Event*>(e.get())->receive_time_ = 26;
                std::shared_ptr<warped::Event> restored_state_event = sm.restoreState(e, 3, lp);
                REQUIRE(sm.size(3) == 3);
                REQUIRE(restored_state_event->timestamp() == 25);
                REQUIRE(static_cast<test_LPState&>(lp->getState()).x == 55);
            }
        }
    }
}

TEST_CASE("States with shared_ptr saved and restored correctly", "[state][queue][shared_ptr]") {
    unsigned int num_lps = 1;
    unsigned int period = 1;
    warped::TimeWarpPeriodicStateManager sm(period);
    sm.initialize(num_lps);
    warped::LogicalProcess *lp;
    std::shared_ptr<warped::Event> e;

    lp = new test_LogicalProcess("test_lp", 50);

    *(static_cast<test_LPState&>(lp->getState()).y) = 5;
    e = std::make_shared<test_Event>();
    dynamic_cast<test_Event*>(e.get())->receive_time_ = 15;
    sm.saveState(e, 0, lp); // timestamp 15
    REQUIRE(sm.size(0) == 1);   // Saved

    *(static_cast<test_LPState&>(lp->getState()).y) = 10;
    e = std::make_shared<test_Event>();
    dynamic_cast<test_Event*>(e.get())->receive_time_ = 30;
    sm.saveState(e, 0, lp); // timestamp 30
    REQUIRE(sm.size(0) == 2);   // Saved

    SECTION("State can be restored on rollback", "[state][rollback][restore]") {
        REQUIRE(sm.size(0) == 2);
        e = std::make_shared<test_Event>();
        dynamic_cast<test_Event*>(e.get())->receive_time_ = 26;
        std::shared_ptr<warped::Event> restored_state_event = sm.restoreState(e, 0, lp);
        REQUIRE(sm.size(0) == 1);
        REQUIRE(restored_state_event->timestamp() == 15);
        REQUIRE(*(static_cast<test_LPState&>(lp->getState()).y) == 5);
    }
}

TEST_CASE("States with maps saved and restored correctly", "[state][queue][map]") {
    unsigned int num_lps = 1;
    unsigned int period = 1;
    warped::TimeWarpPeriodicStateManager sm(period);
    sm.initialize(num_lps);
    warped::LogicalProcess *lp;
    std::shared_ptr<warped::Event> e;

    lp = new test_LogicalProcess("test_lp", 50);

    auto val1 = std::make_shared<enum_type>(VALUE1);
    static_cast<test_LPState&>(lp->getState()).a_map->insert(std::pair<unsigned int, std::shared_ptr<enum_type>>(1, val1));
    e = std::make_shared<test_Event>();
    dynamic_cast<test_Event*>(e.get())->receive_time_ = 15;
    sm.saveState(e, 0, lp); // timestamp 15
    REQUIRE(sm.size(0) == 1);   // Saved

    auto val2 = std::make_shared<enum_type>(VALUE2);
    static_cast<test_LPState&>(lp->getState()).a_map->insert(std::pair<unsigned int, std::shared_ptr<enum_type>>(2, val2));
    e = std::make_shared<test_Event>();
    dynamic_cast<test_Event*>(e.get())->receive_time_ = 30;
    sm.saveState(e, 0, lp); // timestamp 30
    REQUIRE(sm.size(0) == 2);   // Saved

    SECTION("State can be restored on rollback", "[state][rollback][restore]") {
        REQUIRE(sm.size(0) == 2);
        REQUIRE(static_cast<test_LPState&>(lp->getState()).a_map->size() == 2);
        REQUIRE(*(static_cast<test_LPState&>(lp->getState()).a_map->at(1)) == VALUE1);
        REQUIRE(*(static_cast<test_LPState&>(lp->getState()).a_map->at(2)) == VALUE2);

        e = std::make_shared<test_Event>();
        dynamic_cast<test_Event*>(e.get())->receive_time_ = 26;
        std::shared_ptr<warped::Event> restored_state_event = sm.restoreState(e, 0, lp);

        REQUIRE(sm.size(0) == 1);
        REQUIRE(restored_state_event->timestamp() == 15);
        REQUIRE(static_cast<test_LPState&>(lp->getState()).a_map->size() == 1);
        REQUIRE(*(static_cast<test_LPState&>(lp->getState()).a_map->at(1)) == VALUE1);
        REQUIRE_THROWS(static_cast<test_LPState&>(lp->getState()).a_map->at(2));
    }
}

