#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>
#include <vector>

#include "RoundRobinPartitioner.hpp"
#include "SimulationObject.hpp"

TEST_CASE("The RoundRobinPartitioner partitions correctly", "[RoundRobinPartitioner]") {
    std::vector<warped::SimulationObject*> objects;
    warped::RoundRobinPartitioner p;

    SECTION("two partitions") {
        SECTION("two objects") {
            objects = {nullptr, nullptr};
            auto parts = p.partition(objects, 2);
            REQUIRE(parts.size() == 2);
            CHECK(parts[0].size() == 1);
            CHECK(parts[1].size() == 1);
        }
        SECTION("three objects") {
            objects = {nullptr, nullptr, nullptr};
            auto parts = p.partition(objects, 2);
            REQUIRE(parts.size() == 2);
            CHECK(parts[0].size() == 2);
            CHECK(parts[1].size() == 1);
        }
        SECTION("four objects") {
            objects = {nullptr, nullptr, nullptr, nullptr};
            auto parts = p.partition(objects, 2);
            REQUIRE(parts.size() == 2);
            CHECK(parts[0].size() == 2);
            CHECK(parts[1].size() == 2);
        }
    }
    
    SECTION("three partitions") {
        SECTION("three objects") {
            objects = {nullptr, nullptr, nullptr};
            auto parts = p.partition(objects, 3);
            REQUIRE(parts.size() == 3);
            CHECK(parts[0].size() == 1);
            CHECK(parts[1].size() == 1);
            CHECK(parts[2].size() == 1);
        }
        SECTION("four objects") {
            objects = {nullptr, nullptr, nullptr, nullptr};
            auto parts = p.partition(objects, 3);
            REQUIRE(parts.size() == 3);
            CHECK(parts[0].size() == 2);
            CHECK(parts[1].size() == 1);
            CHECK(parts[2].size() == 1);
        }
        SECTION("six objects") {
            objects = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
            auto parts = p.partition(objects, 3);
            REQUIRE(parts.size() == 3);
            CHECK(parts[0].size() == 2);
            CHECK(parts[1].size() == 2);
            CHECK(parts[2].size() == 2);
        }
    }

}

