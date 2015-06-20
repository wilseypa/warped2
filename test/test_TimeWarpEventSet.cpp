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
    twes.initialize(num_objects, num_schedulers, false, num_threads);
    std::shared_ptr<warped::Event> spe, restored_event;
    REQUIRE(twes.getEvent(0) == nullptr);

    SECTION("Insert event (+ve, -ve), get event and start scheduling events") {

        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 5}));
        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 6}));

        restored_event = twes.getEvent(0);
        CHECK(restored_event != nullptr);
        CHECK(restored_event->timestamp() == 5);
        CHECK(restored_event->event_type_ == warped::EventType::POSITIVE);
        twes.replenishScheduler(0);

        spe = twes.getEvent(0);
        CHECK(spe != nullptr);
        CHECK(spe->timestamp() == 6);
        CHECK(spe->event_type_ == warped::EventType::POSITIVE);
        twes.replenishScheduler(0);

        spe = twes.getEvent(0);
        CHECK(spe == nullptr);

        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 16}));
        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 10}));
        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 15}));
        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 10, false}));
        spe = twes.getEvent(0);
        CHECK(spe != nullptr);
        CHECK(spe->timestamp() == 10);
        CHECK(spe->event_type_ == warped::EventType::NEGATIVE);
        twes.cancelEvent(0, spe);
        twes.startScheduling(0);

        spe = twes.lastProcessedEvent(0);
        CHECK(spe != nullptr);
        CHECK(spe->timestamp() == 6);
        CHECK(spe->event_type_ == warped::EventType::POSITIVE);

        spe = twes.getEvent(0);
        CHECK(spe != nullptr);
        CHECK(spe->timestamp() == 15);
        CHECK(spe->event_type_ == warped::EventType::POSITIVE);
        twes.replenishScheduler(0);

        spe = twes.lastProcessedEvent(0);
        CHECK(spe != nullptr);
        CHECK(spe->timestamp() == 15);
        CHECK(spe->event_type_ == warped::EventType::POSITIVE);

        twes.insertEvent(0, std::shared_ptr<warped::Event>(new test_Event {"a", 9}));
        spe = twes.getEvent(0);
        CHECK(spe != nullptr);
        CHECK(spe->timestamp() == 9);
        CHECK(spe->event_type_ == warped::EventType::POSITIVE);

        twes.rollback(0, spe);
        auto list_obj = twes.getEventsForCoastForward(0, spe, restored_event);
        CHECK(list_obj->size() == 1);
        CHECK((*list_obj)[0]->timestamp() == 6);
        twes.startScheduling(0);

        spe = twes.getEvent(0);
        CHECK(spe != nullptr);
        CHECK(spe->timestamp() == 9);
        CHECK(spe->event_type_ == warped::EventType::POSITIVE);
        twes.replenishScheduler(0);

        spe = twes.getEvent(0);
        CHECK(spe != nullptr);
        CHECK(spe->timestamp() == 15);
        CHECK(spe->event_type_ == warped::EventType::POSITIVE);
    }
}
