#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <memory>
#include <vector>

#include "tclap/ValueArg.h"

#include "Simulation.hpp"

TEST_CASE("The Simulation can be constructed without command line parsing", "[Simulation]") {
    std::unique_ptr<warped::Simulation> s;

    REQUIRE_NOTHROW((s = decltype(s){new warped::Simulation {"", 100}}));
}

