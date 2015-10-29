#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <sstream>
#include <vector>
#include <algorithm>
#include <string>

#include "ProfileGuidedPartitioner.hpp"
#include "LogicalProcess.hpp"
#include "mocks.hpp"

TEST_CASE("ProfileGuidedPartitioner partitions fully connected graph", "[partitioning]") {
    std::ofstream of("profile_guided_stats.test", std::ios_base::trunc);
    of << "2 1 001\n%: ob1\n2 1\n%: ob2\n1 1\n";
    of.close();

    unsigned int num_parts = 2;

    test_LogicalProcess lp1{"ob1"};
    test_LogicalProcess lp2{"ob2"};
    std::vector<warped::LogicalProcess*> lps = {&lp1, &lp2};

    auto partitions = warped::ProfileGuidedPartitioner("profile_guided_stats.test", "partition").partition(lps, num_parts);

    SECTION("correct number of partitions") {
        REQUIRE(partitions.size() == num_parts);
    }

    std::vector<warped::LogicalProcess*> partitioned_lps;
    for (int i = 0; i < num_parts; ++i) {
        for (auto lp : partitions[i]) {
            partitioned_lps.push_back(lp);
        }
    }

    SECTION("all lps contained") {
        REQUIRE(partitioned_lps.size() == lps.size());
    }

    SECTION("all lps valid") {
        for (auto lp : partitioned_lps) {
            REQUIRE(std::find(lps.begin(), lps.end(), lp) != lps.end());
        }
    }

    SECTION("all lps unique") {
        REQUIRE(partitioned_lps[0] != partitioned_lps[1]);
    }

}

TEST_CASE("ProfileGuidedPartitioner partitions graph with extra nodes", "[partitioning]") {
    std::ofstream of("profile_guided_stats.test", std::ios_base::trunc);
    of << "2 1 001\n%: lp1\n2 1\n%: lp2\n1 1\n";
    of.close();

    int num_parts = 2;

    test_LogicalProcess lp1{"lp1"};
    test_LogicalProcess lp2{"lp2"};
    test_LogicalProcess lp3{"lp3"};
    std::vector<warped::LogicalProcess*> lps = {&lp1, &lp2, &lp3};

    auto partitions = warped::ProfileGuidedPartitioner("profile_guided_stats.test", "partition").partition(lps, num_parts);

    SECTION("correct number of partitions") {
        REQUIRE(partitions.size() == num_parts);
    }

    std::vector<warped::LogicalProcess*> partitioned_lps;
    for (int i = 0; i < num_parts; ++i) {
        for (auto lp : partitions[i]) {
            partitioned_lps.push_back(lp);
        }
    }

    SECTION("all lps contained") {
        REQUIRE(partitioned_lps.size() == lps.size());
    }

    SECTION("all objects valid") {
        for (auto lp : partitioned_lps) {
            REQUIRE(std::find(lps.begin(), lps.end(), lp) != lps.end());
        }
    }

    SECTION("all lps unique") {
        CHECK(partitioned_lps[0] != partitioned_lps[1]);
        CHECK(partitioned_lps[0] != partitioned_lps[2]);
        CHECK(partitioned_lps[1] != partitioned_lps[2]);
    }

}
