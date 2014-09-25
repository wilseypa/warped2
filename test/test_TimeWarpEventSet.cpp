#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>
#include <sstream>

#include "Event.hpp"
#include "TimeWarpEventSet.hpp"
#include "mocks.hpp"

TEST_CASE("Test the event set operations") {

    unsigned int num_objects = 4, num_schedulers = 1, num_threads = 1;
    warped::TimeWarpEventSet twes;
    twes.initialize(num_objects, num_schedulers, num_threads);
    std::shared_ptr<warped::Event> spe;
    REQUIRE(twes.getEvent(0) == nullptr);

    SECTION("Insert and start scheduling events") {
        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 15}));
        twes.insertEvent(1, std::shared_ptr<warped::Event>(new test_Event {"b", 10}));
        twes.startScheduling(0);
        twes.startScheduling(1);
        spe = twes.getEvent(0);
        REQUIRE(spe != nullptr);
        CHECK(spe->receiverName() == "b");
        CHECK(spe->timestamp() == 10);
    }

    SECTION("get lowest event from object") {
        REQUIRE(twes.getEvent(0) == nullptr);
        REQUIRE(twes.getEvent(1) == nullptr);
        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 15}));
        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 14}));
        spe = twes.getLowestEventFromObj(0);
        REQUIRE(spe != nullptr);
        CHECK(spe->receiverName() == "a");
        CHECK(spe->timestamp() == 14);
        twes.startScheduling(0);
        spe = twes.getLowestEventFromObj(0);
        REQUIRE(spe != nullptr);
        CHECK(spe->receiverName() == "a");
        CHECK(spe->timestamp() == 15);
    }

}
