#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>
#include <sstream>

#include "Event.hpp"
#include "mocks.hpp"
#include "serialization.hpp"

TEST_CASE("Events can be serialized", "[serialization][Event]") {
    std::stringstream ss;
    std::unique_ptr<warped::Event> e {new test_Event{"e", 1, 2}};
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
    REQUIRE(pe->receiver_name_ == "e");
    REQUIRE(pe->receive_time_ == 1);
    REQUIRE(pe->x == 2);
}