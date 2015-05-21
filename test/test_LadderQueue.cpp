#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>
#include <utility>

#include "LadderQueue.hpp"
#include "Event.hpp"
#include "mocks.hpp"

TEST_CASE("Ladder Queue operations") {

    warped::LadderQueue q;
    std::shared_ptr<warped::Event> e;
    REQUIRE(q.begin() == nullptr);

    SECTION("Adding events individually") {
        q.insert(std::shared_ptr<warped::Event>(new test_Event {"1", 10}));
        e = q.begin();
        REQUIRE(e != nullptr);
        CHECK(e->receiverName() == "1");
        CHECK(e->timestamp() == 10);
        q.erase(e);
        e = q.begin();
        REQUIRE(e == nullptr);

#if 0
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
#endif
    }
}
