#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>
#include <vector>

#include "RoundRobinPartitioner.hpp"
#include "LogicalProcess.hpp"

TEST_CASE("The RoundRobinPartitioner partitions correctly", "[RoundRobinPartitioner]") {
    std::vector<warped::LogicalProcess*> lps;
    warped::RoundRobinPartitioner p(1);

    SECTION("two partitions") {
        SECTION("two lps") {
            lps = {nullptr, nullptr};
            auto parts = p.partition(lps, 2);
            REQUIRE(parts.size() == 2);
            CHECK(parts[0].size() == 1);
            CHECK(parts[1].size() == 1);
        }
        SECTION("three lps") {
            lps = {nullptr, nullptr, nullptr};
            auto parts = p.partition(lps, 2);
            REQUIRE(parts.size() == 2);
            CHECK(parts[0].size() == 2);
            CHECK(parts[1].size() == 1);
        }
        SECTION("four lps") {
            lps = {nullptr, nullptr, nullptr, nullptr};
            auto parts = p.partition(lps, 2);
            REQUIRE(parts.size() == 2);
            CHECK(parts[0].size() == 2);
            CHECK(parts[1].size() == 2);
        }
    }

    SECTION("three partitions") {
        SECTION("three lps") {
            lps = {nullptr, nullptr, nullptr};
            auto parts = p.partition(lps, 3);
            REQUIRE(parts.size() == 3);
            CHECK(parts[0].size() == 1);
            CHECK(parts[1].size() == 1);
            CHECK(parts[2].size() == 1);
        }
        SECTION("four lps") {
            lps = {nullptr, nullptr, nullptr, nullptr};
            auto parts = p.partition(lps, 3);
            REQUIRE(parts.size() == 3);
            CHECK(parts[0].size() == 2);
            CHECK(parts[1].size() == 1);
            CHECK(parts[2].size() == 1);
        }
        SECTION("six lps") {
            lps = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
            auto parts = p.partition(lps, 3);
            REQUIRE(parts.size() == 3);
            CHECK(parts[0].size() == 2);
            CHECK(parts[1].size() == 2);
            CHECK(parts[2].size() == 2);
        }
    }

}

