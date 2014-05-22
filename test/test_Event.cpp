#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>

#include "serialization.hpp"
#include "Event.hpp"


struct test_Event : public warped::Event {
    test_Event() = default;
    test_Event(const std::string& receiver_name, unsigned int receive_time, int x)
        : receiver_name_(receiver_name), receive_time_(receive_time), x(x) {}

    const std::string& get_receiver_name() const {return receiver_name_;}
    unsigned int get_receive_time() const {return receive_time_;}

    int x;
    std::string receiver_name_;
    unsigned int receive_time_;

    WARPED_REGISTER_SERIALIZABLE_MEMBERS(receiver_name_, receive_time_, x);
};
WARPED_REGISTER_POLYMORPHIC_SERIALIZABLE_CLASS(test_Event);

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