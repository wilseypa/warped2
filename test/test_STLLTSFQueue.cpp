#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>

#include "STLLTSFQueue.hpp"
#include "Event.hpp"

struct test_Event : public warped::Event {
    test_Event(const std::string& receiver_name, unsigned int receive_time)
        : receiver_name_(receiver_name), receive_time_(receive_time) {}

    const std::string& get_receiver_name() { return receiver_name_; }
    unsigned int get_receive_time() { return receive_time_; }

    std::string receiver_name_;
    unsigned int receive_time_;
};

TEST_CASE("Basic Queue Functionality", "[STLLTSFQueue][LTSFQueue]") {
    warped::STLLTSFQueue q;
    test_Event* e;
    std::unique_ptr<warped::Event> upe;
    REQUIRE(q.empty());
    REQUIRE(q.size() == 0);
    REQUIRE(q.peek() == nullptr);

    SECTION("added events are sorted by timestamp") {
        q.push(std::unique_ptr<warped::Event>(new test_Event {"1", 10}));
        REQUIRE(!q.empty());
        REQUIRE(q.size() == 1);
        REQUIRE(q.peek() != nullptr);

        e = dynamic_cast<test_Event*>(q.peek());
        REQUIRE(e != nullptr);
        CHECK(e->get_receiver_name() == "1");
        CHECK(e->get_receive_time() == 10);

        // Add a second event with a smaller timestamp
        q.push(std::unique_ptr<warped::Event>(new test_Event {"2", 5}));
        CHECK(q.size() == 2);
        REQUIRE(q.peek() != nullptr);

        e = dynamic_cast<test_Event*>(q.peek());
        REQUIRE(e != nullptr);
        CHECK(e->get_receiver_name() == "2");
        CHECK(e->get_receive_time() == 5);

        // Add a third event with a larger timestamp
        q.push(std::unique_ptr<warped::Event>(new test_Event {"3", 15}));
        CHECK(q.size() == 3);
        REQUIRE(q.peek() != nullptr);

        e = dynamic_cast<test_Event*>(q.peek());
        REQUIRE(e != nullptr);
        CHECK(e->get_receiver_name() == "2");
        CHECK(e->get_receive_time() == 5);

        // pop all three events
        int expected_timestamps[3] = {5, 10, 15};

        for (int i = 0; i < 3; ++i) {
            upe = q.pop();
            CHECK(q.size() == (2-i));
            e = dynamic_cast<test_Event*>(upe.get());
            REQUIRE(e != nullptr);
            CHECK(e->get_receive_time() == expected_timestamps[i]);
        }

        REQUIRE(q.peek() == nullptr);
        REQUIRE(q.empty());
    }
}