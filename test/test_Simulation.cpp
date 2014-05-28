#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main()
#include "catch.hpp"

#include <vector>

#include "tclap/ValueArg.h"

#include "Simulation.hpp"

TEST_CASE("The Simulation can be constructed without command line parsing", "[Simulation]") {
    warped::Simulation* s = nullptr;

    REQUIRE_NOTHROW((s = new warped::Simulation {"", 100}));

    delete s;
}

