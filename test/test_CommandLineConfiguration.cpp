#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <utility>
#include <vector>

#include "json/json.h"
#include "tclap/ValueArg.h"

#include "CommandLineConfiguration.hpp"

TEST_CASE("The CommandLineConfiguration parses arguments correctly", "[CommandLineConfiguration]") {
    Json::Value root;
    Json::Reader reader;
    reader.parse("{}", root);
    warped::CommandLineConfiguration config {root};
    std::vector<TCLAP::Arg*> user_args;

    SECTION("no command line arguments") {
        std::vector<const char*> v {"prog"};
        auto ret = config.parse("model", (int)v.size(), v.data(), user_args);
        CHECK(ret.first == false);
        REQUIRE(ret.second.empty());
    }

    SECTION("short-form default command line arguments") {
        std::vector<const char*> v {"prog", "-c", "test_filename"};
        auto ret = config.parse("model", (int)v.size(), v.data(), user_args);
        REQUIRE(ret.second == "test_filename");
    }

    SECTION("long-form default command line arguments") {
        std::vector<const char*> v {"prog", "--config", "test_filename", "--dump-config"};
        auto ret = config.parse("model", (int)v.size(), v.data(), user_args);
        CHECK(ret.first == true);
        REQUIRE(ret.second == "test_filename");
    }

    SECTION("Flat JSON command line arguments") {
        root["s"] = "s_val";
        root["b"] = false;
        root["d"] = 1.0;
        root["i"] = 1;
        std::vector<const char*> v {"prog", "--s", "new_val", "--b", "1", "--d", "2.0", "--i", "2"};
        config.parse("model", (int)v.size(), v.data(), user_args);
        SECTION("string value") {
            REQUIRE(root["s"].isString());
            REQUIRE(root["s"].asString() == "new_val");
        }
        SECTION("bool value") {
            REQUIRE(root["b"].isBool());
            REQUIRE(root["b"].asBool() == true);
        }
        SECTION("double value") {
            REQUIRE(root["d"].isDouble());
            REQUIRE(root["d"].asDouble() == 2.0);
        }
        SECTION("int value") {
            REQUIRE(root["i"].isInt());
            REQUIRE(root["i"].asInt() == 2);
        }
    }

    SECTION("Nested JSON command line arguments") {
        root["a"] = 2;
        root["b"]["b"] = 3;
        root["b"]["c"]["c"] = 4;

        std::vector<const char*> v {"prog", "--a", "22", "--b-b", "33", "--b-c-c", "44"};
        config.parse("model", (int)v.size(), v.data(), user_args);
        SECTION("no nesting") {
            REQUIRE(root["a"].isInt());
            REQUIRE(root["a"].asInt() == 22);
        }
        SECTION("one nesting level") {
            REQUIRE(root["b"]["b"].isInt());
            REQUIRE(root["b"]["b"].asInt() == 33);
        }
        SECTION("two nesting levels") {
            REQUIRE(root["b"]["c"]["c"].isInt());
            REQUIRE(root["b"]["c"]["c"].asInt() == 44);
        }
    }

    SECTION("User custom arguments") {
        TCLAP::ValueArg<int> arg("a", "arg", "", true, 0, "int");
        user_args.push_back(&arg);
        std::vector<const char*> v {"prog", "-a", "42"};
        REQUIRE_NOTHROW(config.parse("model", (int)v.size(), v.data(), user_args));
        REQUIRE(arg.getValue() == 42);
    }
}


