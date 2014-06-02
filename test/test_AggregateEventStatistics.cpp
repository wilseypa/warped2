#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>
#include <vector>

#include "AggregateEventStatistics.hpp"
#include "mocks.hpp"

TEST_CASE("AggregateEventStatistics records stats", "[AggregateEventStatistics]") {
    warped::AggregateEventStatistics stats {"", warped::AggregateEventStatistics::OutputType::Metis};
    test_Event e {"a", 1};
    stats.record("b", 1, &e);

    SECTION("single edge") {
        REQUIRE(stats.getStat(stats.makeEdge("a", "b")) == 1);
    }

    SECTION("single edge twice") {
        stats.record("b", 1, &e);
        REQUIRE(stats.getStat(stats.makeEdge("a", "b")) == 2);
    }

    SECTION("two edges") {
        stats.record("c", 1, &e);
        REQUIRE(stats.getStat(stats.makeEdge("a", "b")) == 1);
        REQUIRE(stats.getStat(stats.makeEdge("a", "c")) == 1);
    }

    SECTION("edges are undirected") {
        test_Event e2 {"b", 1};
        stats.record("a", 1, &e2);

        REQUIRE(stats.getStat(stats.makeEdge("a", "b")) == 2);
    }
}

