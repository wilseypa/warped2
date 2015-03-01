#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>
#include <utility>

#include "STLLTSFQueue.hpp"
#include "Event.hpp"
#include "mocks.hpp"

TEST_CASE("Queue events are sorted by timestamp", "[STLLTSFQueue][LTSFQueue]") {
    warped::STLLTSFQueue q;
    std::shared_ptr<warped::Event> spe;
    REQUIRE(q.empty());
    REQUIRE(q.size() == 0);
    REQUIRE(q.peek() == nullptr);

    SECTION("Adding events individually") {
        q.push(std::shared_ptr<warped::Event>(new test_Event {"1", 10}));
        REQUIRE(!q.empty());
        REQUIRE(q.size() == 1);
        REQUIRE(q.peek() != nullptr);

        spe = q.peek();
        REQUIRE(spe != nullptr);
        CHECK(spe->receiverName() == "1");
        CHECK(spe->timestamp() == 10);

        // Add a second event with a smaller timestamp
        q.push(std::shared_ptr<warped::Event>(new test_Event {"2", 5}));
        CHECK(q.size() == 2);
        REQUIRE(q.peek() != nullptr);

        spe = q.peek();
        REQUIRE(spe != nullptr);
        CHECK(spe->receiverName() == "2");
        CHECK(spe->timestamp() == 5);

        // Add a third event with a larger timestamp
        q.push(std::shared_ptr<warped::Event>(new test_Event {"3", 15}));
        CHECK(q.size() == 3);
        REQUIRE(q.peek() != nullptr);

        spe = q.peek();
        REQUIRE(spe != nullptr);
        CHECK(spe->receiverName() == "2");
        CHECK(spe->timestamp() == 5);

        // pop all three events
        int expected_timestamps[3] = {5, 10, 15};

       for (int i = 0; i < 3; ++i) {
            spe = q.pop();
            CHECK(q.size() == (2 - i));
            REQUIRE(spe != nullptr);
            CHECK(spe->timestamp() == expected_timestamps[i]);
        }

        REQUIRE(q.peek() == nullptr);
        REQUIRE(q.empty());
    }

    SECTION("Adding events in a vector") {
        std::vector<std::shared_ptr<warped::Event>> v;
        v.emplace_back(new test_Event {"1", 10});
        v.emplace_back(new test_Event {"2", 5});
        v.emplace_back(new test_Event {"3", 15});

        q.push(std::move(v));
        REQUIRE(!q.empty());
        REQUIRE(q.size() == 3);
        REQUIRE(q.peek() != nullptr);

        // pop all three events
        int expected_timestamps[3] = {5, 10, 15};

        for (int i = 0; i < 3; ++i) {
            spe = q.pop();
            CHECK(q.size() == (2 - i));
            REQUIRE(spe != nullptr);
            CHECK(spe->timestamp() == expected_timestamps[i]);
        }

        REQUIRE(q.peek() == nullptr);
        REQUIRE(q.empty());
    }

    SECTION("Adding multiple events with the same timestamp") {
        std::vector<std::shared_ptr<warped::Event>> v;
        v.emplace_back(new test_Event {"1", 10});
        v.emplace_back(new test_Event {"2", 5});
        v.emplace_back(new test_Event {"3", 10});

        q.push(std::move(v));
        REQUIRE(!q.empty());
        REQUIRE(q.size() == 3);
        REQUIRE(q.peek() != nullptr);

        // pop all three events
        int expected_timestamps[3] = {5, 10, 10};

        for (int i = 0; i < 3; ++i) {
            spe = q.pop();
            CHECK(q.size() == (2 - i));
            REQUIRE(spe != nullptr);
            CHECK(spe->timestamp() == expected_timestamps[i]);
        }

        REQUIRE(q.peek() == nullptr);
        REQUIRE(q.empty());
    }
}
