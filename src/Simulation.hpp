#ifndef SIMULATION_H
#define SIMULATION_H

#include <memory>
#include <string>
#include <vector>

#include "SimulationObject.hpp"

namespace TCLAP { class Arg; }

namespace warped {

class Simulation {
public:
    Simulation(const std::string& model_description, int argc, const char* const* argv);
    Simulation(const std::string& model_description, int argc, const char* const* argv,
               const std::vector<TCLAP::Arg*>& cmd_line_args);
    ~Simulation();

    // void simulate(std::vector<SimulationObject> objects); // TODO
    // void simulate(std::vector<std::vector<SimulationObject>> objects); 

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace warped

#endif

