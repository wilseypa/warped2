#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>
#include <sstream>
#include <vector>

#include "json/json.h"

#include "Event.hpp"
#include "IndividualEventStatistics.hpp"
#include "utility/memory.hpp"
#include "mocks.hpp"

TEST_CASE("IndividualEventStatistics can be reported as JSON", "[IndividualEventStatistics]") {
    auto type = warped::IndividualEventStatistics::OutputType::Json;
    warped::IndividualEventStatistics stats{"", type};
    Json::Value val;

    std::unique_ptr<warped::Event> e1 = warped::make_unique<test_Event>("e1", 1);
    std::unique_ptr<warped::Event> e2 = warped::make_unique<test_Event>("e2", 2);

    stats.record("s1", 0, e1.get());
    stats.record("s2", 1, e2.get());

    std::stringstream ss;
    ss << stats;
    ss >> val;

    REQUIRE(val.isArray());
    REQUIRE(val[0]["source"].isString());
    REQUIRE(val[0]["dest"].isString());
    REQUIRE(val[0]["send_time"].isInt());
    REQUIRE(val[0]["receive_time"].isInt());
    CHECK(val[0]["source"].asString() == "s1");
    CHECK(val[0]["dest"].asString() == "e1");
    CHECK(val[0]["send_time"].asInt() == 0);
    CHECK(val[0]["receive_time"].asInt() == 1);

    REQUIRE(val[1]["source"].isString());
    REQUIRE(val[1]["dest"].isString());
    REQUIRE(val[1]["send_time"].isInt());
    REQUIRE(val[1]["receive_time"].isInt());
    CHECK(val[1]["source"].asString() == "s2");
    CHECK(val[1]["dest"].asString() == "e2");
    CHECK(val[1]["send_time"].asInt() == 1);
    CHECK(val[1]["receive_time"].asInt() == 2);
}

