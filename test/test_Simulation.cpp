#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <vector>

#include "tclap/ValueArg.h"

#include "Simulation.hpp"

TEST_CASE("The Simulation parses command line arguments", "[Simulation]") {
    warped::Simulation* s = nullptr;
    std::vector<const char*> v {"prog", "-c", "filename", "-u", "100"};

    SECTION("Simulation supports default arguments") {
        REQUIRE_NOTHROW((s = new warped::Simulation {"model", (int)v.size(), v.data()}));
    }

    SECTION("Simulation supports custom arguments") {
        TCLAP::ValueArg<int> arg("a", "arg", "", true, 0, "int");
        std::vector<TCLAP::Arg*> v2 {&arg};

        v.push_back("-a");
        v.push_back("42");

        REQUIRE_NOTHROW((s = new warped::Simulation {"model", (int)v.size(), v.data(), v2}));
        REQUIRE(arg.getValue() == 42);
    }

    delete s;
}

TEST_CASE("The Simulation can be constructed without command line parsing", "[Simulation]") {
    warped::Simulation* s = nullptr;

    REQUIRE_NOTHROW((s = new warped::Simulation {"filename", 100}));

    delete s;
}