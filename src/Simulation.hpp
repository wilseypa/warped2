#ifndef SIMULATION_H
#define SIMULATION_H

#include "tclap/Arg.h"

#include <vector>

namespace warped {

class Simulation {
public:
    Simulation(int argc, char* argv[]);
    Simulation(int argc, char* argv[], std::vector<TCLAP::Arg> options);

};

} // namespace warped

#endif

