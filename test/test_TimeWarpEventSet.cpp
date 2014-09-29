#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>
#include <sstream>

#include "Event.hpp"
#include "TimeWarpEventDispatcher.hpp"
#include "TimeWarpEventSet.hpp"
#include "mocks.hpp"

TEST_CASE("Test the event set operations") {

    unsigned int num_objects = 4, num_schedulers = 1, num_threads = 1;
    warped::TimeWarpEventSet twes;
    twes.initialize(num_objects, num_schedulers, num_threads);
    std::shared_ptr<warped::Event> spe;
    REQUIRE(twes.getEvent(0) == nullptr);

    SECTION("Insert event (+ve, -ve), get event and start scheduling events") {
        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 15}));
        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 10}));
        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 11}));
        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 10, false}));
        twes.insertEvent(1, std::shared_ptr<warped::Event>(new test_Event {"b", 9}));
        spe = twes.getEvent(0);
        REQUIRE(spe != nullptr);
        CHECK(spe->receiverName() == "b");
        CHECK(spe->timestamp() == 9);
        spe = twes.getEvent(0);
        REQUIRE(spe != nullptr);
        CHECK(spe->receiverName() == "a");
        CHECK(spe->timestamp() == 15);
        twes.replenishScheduler(0, spe);
        spe = twes.getEvent(0);
        REQUIRE(spe != nullptr);
        CHECK(spe->receiverName() == "a");
        CHECK(spe->timestamp() == 11);
    }
}
