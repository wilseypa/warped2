#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>
#include <sstream>

#include "Event.hpp"
#include "TimeWarpEventDispatcher.hpp"
#include "mocks.hpp"
#include "serialization.hpp"
#include "utility/memory.hpp"

TEST_CASE("Events can be serialized", "[serialization][Event]") {
    std::stringstream ss;
    std::unique_ptr<warped::Event> e {new test_Event{"receiver", "sender", 1, 2}};
    std::unique_ptr<warped::Event> e2;

    {
        cereal::BinaryOutputArchive oarchive(ss);
        oarchive(e);
    }

    {
        cereal::BinaryInputArchive iarchive(ss);
        iarchive(e2);
    }
    auto pe = dynamic_cast<test_Event*>(e2.get());
    REQUIRE(pe != nullptr);
    REQUIRE(pe->receiver_name_ == "receiver");
    REQUIRE(pe->sender_name_ == "sender");
    REQUIRE(pe->receive_time_ == 1);
    REQUIRE(pe->x == 2);
}

TEST_CASE("Events can be compared from base class", "[Compare][Event]") {
    std::unique_ptr<warped::Event> e1 = warped::make_unique<test_Event>
        ("receiver1", "sender1", 1, 2);
    std::unique_ptr<warped::Event> e2 = warped::make_unique<test_Event>
        ("receiver2", "sender1", 1, 10);
    std::unique_ptr<warped::Event> e3 = warped::make_unique<test_Event>
        ("receiver1", "sender2", 1, 23);
    std::unique_ptr<warped::Event> e4 = warped::make_unique<test_Event>
        ("receiver1", "sender1", 2, 6);
    std::unique_ptr<warped::Event> e5 = warped::make_unique<test_Event>
        ("receiver1", "sender1", 1, 4);

    CHECK(*e1 != *e2);
    CHECK(*e1 != *e3);
    CHECK(*e1 != *e4);
    CHECK(*e1 == *e5);

    SECTION("Negative events can be compared to positive events", "[Negative][Compare][Event]") {
        std::unique_ptr<warped::Event> e6 = warped::make_unique<warped::NegativeEvent>
            ("receiver1", "sender1", 1);

        CHECK(*e6 == *e1);
        CHECK(*e6 != *e2);
        CHECK(*e6 != *e3);
        CHECK(*e6 != *e4);
        CHECK(*e6 == *e5);
    }

}


