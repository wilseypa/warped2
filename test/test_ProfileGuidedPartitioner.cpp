#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <sstream>
#include <vector>
#include <algorithm>
#include <string>

#include "ProfileGuidedPartitioner.hpp"
#include "SimulationObject.hpp"
#include "mocks.hpp"

TEST_CASE("ProfileGuidedPartitioner partitions fully connected graph", "[partitioning]") {
    std::stringstream input;
    input <<
          "2 1 001\n"
          "%: ob1\n"
          "2 1\n"
          "%: ob2\n"
          "1 1\n";

    unsigned int num_parts = 2;

    test_SimulationObject ob1{"ob1"};
    test_SimulationObject ob2{"ob2"};
    std::vector<warped::SimulationObject*> objects = {&ob1, &ob2};

    auto partitions = warped::ProfileGuidedPartitioner(input).partition(objects, num_parts);

    SECTION("correct number of partitions") {
        REQUIRE(partitions.size() == num_parts);
    }

    std::vector<warped::SimulationObject*> partitioned_objects;
    for (int i = 0; i < num_parts; ++i) {
        for (auto ob : partitions[i]) {
            partitioned_objects.push_back(ob);
        }
    }

    SECTION("all objects contained") {
        REQUIRE(partitioned_objects.size() == objects.size());
    }

    SECTION("all objects valid") {
        for (auto ob : partitioned_objects) {
            REQUIRE(std::find(objects.begin(), objects.end(), ob) != objects.end());
        }
    }

    SECTION("all objects unique") {
        REQUIRE(partitioned_objects[0] != partitioned_objects[1]);
    }

}

TEST_CASE("ProfileGuidedPartitioner partitions graph with extra nodes", "[partitioning]") {
    std::stringstream input;
    input <<
          "2 1 001\n"
          "%: ob1\n"
          "2 1\n"
          "%: ob2\n"
          "1 1\n";

    int num_parts = 2;

    test_SimulationObject ob1{"ob1"};
    test_SimulationObject ob2{"ob2"};
    test_SimulationObject ob3{"ob3"};
    std::vector<warped::SimulationObject*> objects = {&ob1, &ob2, &ob3};

    auto partitions = warped::ProfileGuidedPartitioner(input).partition(objects, num_parts);

    SECTION("correct number of partitions") {
        REQUIRE(partitions.size() == num_parts);
    }

    std::vector<warped::SimulationObject*> partitioned_objects;
    for (int i = 0; i < num_parts; ++i) {
        for (auto ob : partitions[i]) {
            partitioned_objects.push_back(ob);
        }
    }

    SECTION("all objects contained") {
        REQUIRE(partitioned_objects.size() == objects.size());
    }

    SECTION("all objects valid") {
        for (auto ob : partitioned_objects) {
            REQUIRE(std::find(objects.begin(), objects.end(), ob) != objects.end());
        }
    }

    SECTION("all objects unique") {
        CHECK(partitioned_objects[0] != partitioned_objects[1]);
        CHECK(partitioned_objects[0] != partitioned_objects[2]);
        CHECK(partitioned_objects[1] != partitioned_objects[2]);
    }

}